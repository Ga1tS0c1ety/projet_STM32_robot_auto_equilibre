
#include <stdint.h>
#include <math.h>

#include "uart.h"
#include "i2c.h"
#include "mpu6050.h"
#include "timebase.h"

#define RAD_TO_DEG 57.2957795f

#define SCB_CPACR (*(volatile uint32_t *)0xE000ED88UL)

/*
 * Période minimale entre deux affichages UART.
 * 100 000 us = 100 ms.
 */
#define UART_PERIOD_US 100000UL

/*
 * Activation du coprocesseur flottant.
 */
static void fpu_init(void)
{
    SCB_CPACR |= (0xFUL << 20);

    __asm volatile ("dsb");
    __asm volatile ("isb");
}

/*
 * Calcul de l'angle à partir de l'accéléromètre.
 *
 * Hypothèse :
 * rotation autour de l'axe Y du MPU6050.
 *
 * Résultat en degrés.
 */
static float calculer_angle_acc(float ax, float az)
{
    return atan2f(-ax, az) * RAD_TO_DEG;
}

/*
 * Affichage des résultats.
 */
static void afficher_angles(float angle_acc,
                            float angle_gyro,
                            float dt)
{
    uart_send_string("Angle ACC  : ");
    uart_send_float(angle_acc, 2);
    uart_send_string(" deg\r\n");

    uart_send_string("Angle GYRO : ");
    uart_send_float(angle_gyro, 2);
    uart_send_string(" deg\r\n");

    uart_send_string("Delta t    : ");
    uart_send_float(dt, 4);
    uart_send_string(" s\r\n\r\n");
}

int main(void)
{
    uint32_t erreur;

    int16_t ax, ay, az;
    int16_t gx, gy, gz;

    int16_t offset_gx;
    int16_t offset_gy;
    int16_t offset_gz;

    float angle_acc;
    float angle_gyro;

    float gy_dps;
    float dt;

    uint32_t temps_precedent;
    uint32_t temps_actuel;
    uint32_t temps_affichage;

    /*
     * 1. Initialisations
     */
    fpu_init();

    uart_init();
    i2c1_init();
    timebase_init();

    uart_send_string(
        "\r\nSeance 8 - Integration gyroscope\r\n"
    );

    erreur = mpu6050_init();

    if (erreur != 0U)
    {
        uart_send_string("Erreur MPU6050\r\n");

        while (1)
        {
        }
    }

    uart_send_string("MPU6050 initialise\r\n");

    /*
     * 2. Calibration du gyroscope.
     * Le MPU6050 doit rester immobile.
     */
    uart_send_string("Calibration gyroscope...\r\n");

    erreur = mpu6050_calibrate_gyro(
        &offset_gx,
        &offset_gy,
        &offset_gz
    );

    if (erreur != 0U)
    {
        uart_send_string("Erreur calibration\r\n");

        while (1)
        {
        }
    }

    uart_send_string("Calibration terminee\r\n");

    /*
     * 3. Lecture initiale de l'accéléromètre.
     *
     * On initialise les deux estimations
     * avec le même angle de départ.
     */
    erreur = mpu6050_read_accel(&ax, &ay, &az);

    if (erreur != 0U)
    {
        uart_send_string("Erreur angle initial\r\n");

        while (1)
        {
        }
    }

    angle_acc = calculer_angle_acc(
        (float)ax,
        (float)az
    );

    angle_gyro = angle_acc;

    /*
     * 4. Référence temporelle.
     */
    temps_precedent = timebase_get_us();
    temps_affichage = temps_precedent;

    uart_send_string("Debut des mesures\r\n\r\n");

    /*
     * 5. Boucle de mesure
     */
    while (1)
    {
        /*
         * Lecture accéléromètre
         */
        erreur = mpu6050_read_accel(&ax, &ay, &az);

        if (erreur != 0U)
        {
            continue;
        }

        /*
         * Calcul de l'angle absolu
         */
        angle_acc = calculer_angle_acc(
            (float)ax,
            (float)az
        );

        /*
         * Lecture gyroscope
         */
        erreur = mpu6050_read_gyro(&gx, &gy, &gz);

        if (erreur != 0U)
        {
            continue;
        }

        /*
         * Mesure du temps écoulé
         */
        temps_actuel = timebase_get_us();

        dt = (float)(temps_actuel - temps_precedent)
             / 1000000.0f;

        temps_precedent = temps_actuel;

        /*
         * Correction du biais du gyroscope.
         *
         * Ici, le tangage est supposé correspondre
         * à la rotation autour de Y.
         */
        int32_t gy_corrige =
            (int32_t)gy - (int32_t)offset_gy;

        gy_dps = mpu6050_gyro_to_dps(
            (int16_t)gy_corrige
        );

        /*
         * 6. Intégration du gyroscope
         *
         * theta(k) = theta(k-1) + omega * dt
         */
        angle_gyro += gy_dps * dt;

        /*
         * 7. Affichage périodique
         *
         * Les mesures continuent à chaque tour.
         * L'UART n'affiche qu'environ 10 fois/s.
         */
        if ((uint32_t)(temps_actuel - temps_affichage)
            >= UART_PERIOD_US)
        {
            afficher_angles(
                angle_acc,
                angle_gyro,
                dt
            );

            uart_send_string("GY vitesse : ");
            uart_send_float(gy_dps, 3);
            uart_send_string(" deg/s\r\n");

            temps_affichage = temps_actuel;
        }
    }
}
