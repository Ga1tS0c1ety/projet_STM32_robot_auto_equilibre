
#include <stdint.h>
#include <math.h>

#include "uart.h"
#include "i2c.h"
#include "mpu6050.h"
#include "timebase.h"

#define RAD_TO_DEG 57.2957795f
#define SCB_CPACR (*(volatile uint32_t *)0xE000ED88UL)

#define UART_PERIOD_US 100000UL

/*
 * Coefficients à comparer.
 */
#define ALPHA_1 0.90f
#define ALPHA_2 0.95f
#define ALPHA_3 0.98f
#define ALPHA_4 0.99f

static void fpu_init(void)
{
    SCB_CPACR |= (0xFUL << 20);

    __asm volatile ("dsb");
    __asm volatile ("isb");
}

/*
 * Angle absolu à partir de l'accéléromètre.
 */
static float calculer_angle_acc(float ax, float az)
{
    return atan2f(-ax, az) * RAD_TO_DEG;
}

/*
 * Filtre complémentaire.
 */
static float filtre_complementaire(float angle_precedent,
                                  float vitesse_gyro,
                                  float angle_acc,
                                  float dt,
                                  float alpha)
{
    float prediction = angle_precedent + vitesse_gyro * dt;

    return alpha * prediction
         + (1.0f - alpha) * angle_acc;
}

/*
 * Affichage d'un angle avec son coefficient.
 */
static void afficher_filtre(const char *nom, float angle)
{
    uart_send_string(nom);
    uart_send_float(angle, 2);
    uart_send_string(" deg\r\n");
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

    float angle_f1;
    float angle_f2;
    float angle_f3;
    float angle_f4;

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
        "\r\n=== COMPARAISON FILTRES COMPLEMENTAIRES ===\r\n"
    );

    erreur = mpu6050_init();

    if (erreur != 0U)
    {
        uart_send_string("Erreur MPU6050\r\n");
        while (1) {}
    }

    /*
     * 2. Calibration du gyroscope
     */
    uart_send_string("Calibration gyro...\r\n");
    uart_send_string("Ne pas bouger le capteur.\r\n");

    erreur = mpu6050_calibrate_gyro(
        &offset_gx,
        &offset_gy,
        &offset_gz
    );

    if (erreur != 0U)
    {
        uart_send_string("Erreur calibration\r\n");
        while (1) {}
    }

    uart_send_string("Calibration terminee\r\n");

    /*
     * 3. Initialisation des angles
     */
    erreur = mpu6050_read_accel(&ax, &ay, &az);

    if (erreur != 0U)
    {
        uart_send_string("Erreur angle initial\r\n");
        while (1) {}
    }

    angle_acc = calculer_angle_acc(
        (float)ax,
        (float)az
    );

    angle_gyro = angle_acc;

    /*
     * Tous les filtres partent du même angle.
     */
    angle_f1 = angle_acc;
    angle_f2 = angle_acc;
    angle_f3 = angle_acc;
    angle_f4 = angle_acc;

    /*
     * 4. Référence temporelle
     */
    temps_precedent = timebase_get_us();
    temps_affichage = temps_precedent;

    uart_send_string("Debut comparaison\r\n\r\n");

    /*
     * 5. Boucle principale
     */
    while (1)
    {
        /*
         * Lire l'accéléromètre.
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
         * Lire le gyroscope.
         */
        erreur = mpu6050_read_gyro(&gx, &gy, &gz);

        if (erreur != 0U)
        {
            continue;
        }

        /*
         * Calculer le temps écoulé.
         */
        temps_actuel = timebase_get_us();

        dt = (float)(temps_actuel - temps_precedent)
             / 1000000.0f;

        temps_precedent = temps_actuel;

        /*
         * Compenser le biais gyroscopique.
         */
        int32_t gy_corrige =
            (int32_t)gy - (int32_t)offset_gy;

        gy_dps = mpu6050_gyro_to_dps(
            (int16_t)gy_corrige
        );

        /*
         * Angle gyroscopique non filtré.
         */
        angle_gyro += gy_dps * dt;

        /*
         * 6. Les quatre filtres.
         */
        angle_f1 = filtre_complementaire(
            angle_f1, gy_dps, angle_acc, dt, ALPHA_1
        );

        angle_f2 = filtre_complementaire(
            angle_f2, gy_dps, angle_acc, dt, ALPHA_2
        );

        angle_f3 = filtre_complementaire(
            angle_f3, gy_dps, angle_acc, dt, ALPHA_3
        );

        angle_f4 = filtre_complementaire(
            angle_f4, gy_dps, angle_acc, dt, ALPHA_4
        );

        /*
         * 7. Affichage toutes les 100 ms.
         *
         * L'intégration et le filtrage continuent
         * à chaque itération.
         */
        if ((uint32_t)(temps_actuel - temps_affichage)
            >= UART_PERIOD_US)
        {
            afficher_filtre("ACC        : ", angle_acc);
            afficher_filtre("GYRO       : ", angle_gyro);

            afficher_filtre("ALPHA 0.90 : ", angle_f1);
            afficher_filtre("ALPHA 0.95 : ", angle_f2);
            afficher_filtre("ALPHA 0.98 : ", angle_f3);
            afficher_filtre("ALPHA 0.99 : ", angle_f4);

            uart_send_string("dt         : ");
            uart_send_float(dt, 4);
            uart_send_string(" s\r\n");

            uart_send_string("------------------------\r\n");

            temps_affichage = temps_actuel;
        }
    }
}
