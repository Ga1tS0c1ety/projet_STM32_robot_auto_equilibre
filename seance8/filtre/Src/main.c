
#include <stdint.h>
#include <math.h>

#include "uart.h"
#include "i2c.h"
#include "mpu6050.h"
#include "timebase.h"

#define RAD_TO_DEG 57.2957795f
#define SCB_CPACR (*(volatile uint32_t *)0xE000ED88UL)

#define FILTRE_ALPHA   0.98f
#define UART_PERIOD_US 100000UL

/*
 * Activation de la FPU Cortex-M4F.
 */
static void fpu_init(void)
{
    SCB_CPACR |= (0xFUL << 20);

    __asm volatile ("dsb");
    __asm volatile ("isb");
}

/*
 * Estimation de l'angle par l'accéléromètre.
 * Rotation supposée autour de l'axe Y.
 */
static float calculer_angle_acc(float ax, float az)
{
    return atan2f(-ax, az) * RAD_TO_DEG;
}

/*
 * Une étape du filtre complémentaire.
 *
 * angle_precedent : estimation filtrée précédente
 * vitesse_gyro    : vitesse angulaire en deg/s
 * dt              : temps écoulé en secondes
 * angle_acc       : angle accélérométrique en degrés
 */
static float filtre_complementaire(float angle_precedent,
                                  float vitesse_gyro,
                                  float angle_acc,
                                  float dt)
{
    float prediction = angle_precedent + vitesse_gyro * dt;

    return FILTRE_ALPHA * prediction
         + (1.0f - FILTRE_ALPHA) * angle_acc;
}

/*
 * Affichage périodique des résultats.
 */
static void afficher_angles(float angle_acc,
                            float angle_gyro,
                            float angle_filtre,
                            float gy_dps,
                            float dt)
{
    uart_send_string("Angle ACC    : ");
    uart_send_float(angle_acc, 2);
    uart_send_string(" deg\r\n");

    uart_send_string("Angle GYRO   : ");
    uart_send_float(angle_gyro, 2);
    uart_send_string(" deg\r\n");

    uart_send_string("Angle FILTRE : ");
    uart_send_float(angle_filtre, 2);
    uart_send_string(" deg\r\n");

    uart_send_string("GY vitesse   : ");
    uart_send_float(gy_dps, 3);
    uart_send_string(" deg/s\r\n");

    uart_send_string("Delta t      : ");
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
    float angle_filtre;

    float gy_dps;
    float dt;

    uint32_t temps_precedent;
    uint32_t temps_actuel;
    uint32_t temps_affichage;

    /*
     * 1. Initialisation matérielle
     */
    fpu_init();

    uart_init();
    i2c1_init();
    timebase_init();

    uart_send_string(
        "\r\n=== SEANCE 8 - FILTRE COMPLEMENTAIRE ===\r\n"
    );

    erreur = mpu6050_init();

    if (erreur != 0U)
    {
        uart_send_string("Erreur initialisation MPU6050 : ");
        uart_send_uint(erreur);
        uart_send_string("\r\n");

        while (1) {}
    }

    uart_send_string("MPU6050 initialise\r\n");

    /*
     * 2. Calibration gyroscope
     *
     * Ne pas bouger le capteur pendant cette phase.
     */
    uart_send_string("Calibration gyroscope...\r\n");
    uart_send_string("Ne pas bouger le MPU6050.\r\n");

    erreur = mpu6050_calibrate_gyro(
        &offset_gx,
        &offset_gy,
        &offset_gz
    );

    if (erreur != 0U)
    {
        uart_send_string("Erreur calibration : ");
        uart_send_uint(erreur);
        uart_send_string("\r\n");

        while (1) {}
    }

    uart_send_string("Calibration terminee\r\n");

    /*
     * 3. Initialisation des estimations
     */
    erreur = mpu6050_read_accel(&ax, &ay, &az);

    if (erreur != 0U)
    {
        uart_send_string("Erreur lecture angle initial\r\n");

        while (1) {}
    }

    angle_acc = calculer_angle_acc(
        (float)ax,
        (float)az
    );

    angle_gyro = angle_acc;
    angle_filtre = angle_acc;

    /*
     * 4. Initialisation temporelle
     */
    temps_precedent = timebase_get_us();
    temps_affichage = temps_precedent;

    uart_send_string("Debut du filtrage\r\n\r\n");

    /*
     * 5. Boucle de mesure
     */
    while (1)
    {
        /*
         * Lecture des accélérations
         */
        erreur = mpu6050_read_accel(&ax, &ay, &az);

        if (erreur != 0U)
        {
            continue;
        }

        angle_acc = calculer_angle_acc(
            (float)ax,
            (float)az
        );

        /*
         * Lecture du gyroscope
         */
        erreur = mpu6050_read_gyro(&gx, &gy, &gz);

        if (erreur != 0U)
        {
            continue;
        }

        /*
         * Mesure précise du temps avec TIM5
         */
        temps_actuel = timebase_get_us();

        dt = (float)(temps_actuel - temps_precedent)
             / 1000000.0f;

        temps_precedent = temps_actuel;

        /*
         * Compensation du biais gyroscopique
         */
        int32_t gy_corrige =
            (int32_t)gy - (int32_t)offset_gy;

        gy_dps = mpu6050_gyro_to_dps(
            (int16_t)gy_corrige
        );

        /*
         * 6. Intégration gyroscope seule
         *
         * Conservée pour comparaison.
         */
        angle_gyro += gy_dps * dt;

        /*
         * 7. Fusion accéléromètre / gyroscope
         */
        angle_filtre = filtre_complementaire(
            angle_filtre,
            gy_dps,
            angle_acc,
            dt
        );

        /*
         * 8. Affichage UART toutes les 100 ms
         *
         * La boucle de mesure tourne indépendamment
         * de l'affichage.
         */
        if ((uint32_t)(temps_actuel - temps_affichage)
            >= UART_PERIOD_US)
        {
            afficher_angles(
                angle_acc,
                angle_gyro,
                angle_filtre,
                gy_dps,
                dt
            );

            temps_affichage = temps_actuel;
        }
    }
}
