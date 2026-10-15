
#include <stdint.h>
#include <math.h>

#include "uart.h"
#include "i2c.h"
#include "mpu6050.h"
#include "timebase.h"
#include "filtre_complementaire.h"

/*
 * Séance 8 — Filtre complémentaire
 *
 * Carte : NUCLEO-F446RE
 * Capteur : MPU6050
 *
 * Axe de rotation étudié : Y
 */

#define RAD_TO_DEG 57.2957795f

#define SCB_CPACR (*(volatile uint32_t *)0xE000ED88UL)

/*
 * Constante de temps du filtre.
 * Environ alpha = 0.98 pour dt = 1.7 ms.
 */
#define FILTRE_TAU 0.083f

/*
 * Affichage UART toutes les 100 ms.
 */
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
 * Calcul de l'angle à partir de l'accéléromètre.
 *
 * L'angle est exprimé en degrés.
 * Convention validée expérimentalement
 * pour la rotation autour de Y.
 */
static float calculer_angle_acc(float ax, float az)
{
    return atan2f(-ax, az) * RAD_TO_DEG;
}

/*
 * Affichage des mesures.
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
    uart_send_string(" s\r\n");

    uart_send_string("------------------------\r\n");
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
     * Etat interne du filtre complémentaire.
     */
    FiltreComplementaire filtre;

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

    /*
     * 2. Initialisation MPU6050
     */
    erreur = mpu6050_init();

    if (erreur != 0U)
    {
        uart_send_string("Erreur initialisation MPU6050\r\n");

        while (1)
        {
        }
    }

    uart_send_string("MPU6050 initialise\r\n");

    /*
     * 3. Calibration du gyroscope
     *
     * Le capteur doit rester immobile.
     */
    uart_send_string("Calibration gyroscope...\r\n");
    uart_send_string("Ne pas bouger le capteur.\r\n");

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
     * 4. Angle initial de l'accéléromètre
     */
    erreur = mpu6050_read_accel(&ax, &ay, &az);

    if (erreur != 0U)
    {
        uart_send_string("Erreur lecture accelerometre\r\n");

        while (1)
        {
        }
    }

    angle_acc = calculer_angle_acc(
        (float)ax,
        (float)az
    );

    /*
     * Le gyroscope et le filtre partent
     * de la même estimation initiale.
     */
    angle_gyro = angle_acc;

    /*
     * 5. Initialisation du filtre complémentaire
     */
    filtre_complementaire_init(
        &filtre,
        angle_acc,
        FILTRE_TAU
    );

    angle_filtre = filtre.angle;

    /*
     * 6. Initialisation de la référence temporelle
     */
    temps_precedent = timebase_get_us();
    temps_affichage = temps_precedent;

    uart_send_string("Debut des mesures\r\n\r\n");

    /*
     * 7. Boucle principale
     */
    while (1)
    {
        /*
         * Lecture de l'accéléromètre
         */
        erreur = mpu6050_read_accel(&ax, &ay, &az);

        if (erreur != 0U)
        {
            continue;
        }

        /*
         * Calcul de l'angle accélérométrique
         */
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
         * Mesure du temps écoulé avec TIM5
         */
        temps_actuel = timebase_get_us();

        dt = (float)(
            temps_actuel - temps_precedent
        ) / 1000000.0f;

        temps_precedent = temps_actuel;

        /*
         * 8. Compensation du biais gyroscopique
         *
         * On soustrait l'offset sur l'axe Y.
         */
        int32_t gy_corrige =
            (int32_t)gy - (int32_t)offset_gy;

        /*
         * Conversion en degrés par seconde.
         *
         * Sensibilité du MPU6050 : 131 LSB/(deg/s)
         * pour la plage configurée à +/-250 deg/s.
         *
         * La conversion est faite directement en float
         * pour éviter tout débordement int16_t après
         * soustraction de l'offset.
         */
        gy_dps = (float)gy_corrige / 131.0f;

        /*
         * 9. Intégration du gyroscope seul
         *
         * Conservée pour la comparaison.
         */
        angle_gyro += gy_dps * dt;

        /*
         * 10. Mise à jour du filtre complémentaire
         *
         * Le module gère son état interne
         * et calcule alpha en fonction de dt.
         */
        angle_filtre = filtre_complementaire_update(
            &filtre,
            gy_dps,
            angle_acc,
            dt
        );

        /*
         * 11. Affichage périodique
         */
        if ((uint32_t)(
            temps_actuel - temps_affichage
        ) >= UART_PERIOD_US)
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
