#include <stdint.h>

#include "uart.h"
#include "i2c.h"
#include "mpu6050.h"


#define SCB_CPACR (*(volatile uint32_t *)0xE000ED88UL)

static void fpu_init(void)
{
    /*
     * Autoriser l'accès complet aux coprocesseurs
     * CP10 et CP11 utilisés par le FPU.
     *
     * CP10 = 11
     * CP11 = 11
     */
    SCB_CPACR |= (0xFUL << 20);

    /*
     * S'assurer que la modification est prise en compte
     * avant d'exécuter une instruction flottante.
     */
    __asm volatile ("dsb");
    __asm volatile ("isb");
}

/*
 * Temporisation approximative.
 *
 * Utilisée uniquement pour ralentir l'affichage UART.
 * Elle ne constitue pas une base de temps précise.
 */
static void delay_approx_ms(uint32_t milliseconds)
{
    for (uint32_t ms = 0; ms < milliseconds; ms++)
    {
        for (volatile uint32_t compteur = 0;
             compteur < 4000U;
             compteur++)
        {
        }
    }
}

/*
 * Affiche les mesures de l'accéléromètre :
 * - valeur brute ;
 * - valeur convertie en g.
 */
static void afficher_accelerometre(int16_t ax,
                                   int16_t ay,
                                   int16_t az,
                                   float ax_g,
                                   float ay_g,
                                   float az_g)
{
    uart_send_string("ACC brut | X = ");
    uart_send_int(ax);

    uart_send_string(" | Y = ");
    uart_send_int(ay);

    uart_send_string(" | Z = ");
    uart_send_int(az);

    uart_send_string("\r\n");

    uart_send_string("ACC g    | X = ");
    uart_send_float(ax_g, 2);

    uart_send_string(" | Y = ");
    uart_send_float(ay_g, 2);

    uart_send_string(" | Z = ");
    uart_send_float(az_g, 2);

    uart_send_string("\r\n");
}

/*
 * Affiche les mesures du gyroscope :
 * - valeur brute ;
 * - valeur convertie en degrés par seconde.
 */
static void afficher_gyroscope(int16_t gx,
                               int16_t gy,
                               int16_t gz,
                               float gx_dps,
                               float gy_dps,
                               float gz_dps)
{
    uart_send_string("GYR brut | X = ");
    uart_send_int(gx);

    uart_send_string(" | Y = ");
    uart_send_int(gy);

    uart_send_string(" | Z = ");
    uart_send_int(gz);

    uart_send_string("\r\n");

    uart_send_string("GYR dps  | X = ");
    uart_send_float(gx_dps, 2);

    uart_send_string(" | Y = ");
    uart_send_float(gy_dps, 2);

    uart_send_string(" | Z = ");
    uart_send_float(gz_dps, 2);

    uart_send_string("\r\n");
}

/*
 * Affiche toutes les mesures de l'IMU.
 */
static void afficher_imu(int16_t ax,
                         int16_t ay,
                         int16_t az,
                         int16_t gx,
                         int16_t gy,
                         int16_t gz,
                         float ax_g,
                         float ay_g,
                         float az_g,
                         float gx_dps,
                         float gy_dps,
                         float gz_dps)
{
    afficher_accelerometre(ax, ay, az,
                          ax_g, ay_g, az_g);

    afficher_gyroscope(gx, gy, gz,
                       gx_dps, gy_dps, gz_dps);

    uart_send_string("\r\n");
}

#define GYRO_CALIBRATION_SAMPLES 500U


int main(void)
{
    uint32_t erreur;

    /*
     * Activer le FPU du Cortex-M4F.
     */
    fpu_init();

    /*
     * Mesures brutes de l'accéléromètre.
     */
    int16_t ax;
    int16_t ay;
    int16_t az;

    /*
     * Mesures brutes du gyroscope.
     */
    int16_t gx;
    int16_t gy;
    int16_t gz;

    /*
     * Offsets statiques du gyroscope.
     */
    int16_t offset_gx;
    int16_t offset_gy;
    int16_t offset_gz;

    /*
     * Mesures converties de l'accéléromètre.
     */
    float ax_g;
    float ay_g;
    float az_g;

    /*
     * Mesures converties du gyroscope.
     */
    float gx_dps;
    float gy_dps;
    float gz_dps;

    /*
     * Initialisation des périphériques STM32.
     */
    uart_init();
    i2c1_init();

    uart_send_string("Demarrage du robot\r\n");

    /*
     * Initialisation du MPU6050.
     */
    erreur = mpu6050_init();

    if (erreur != 0U)
    {
        uart_send_string("Erreur initialisation MPU6050 : ");
        uart_send_uint(erreur);
        uart_send_string("\r\n");

        while (1)
        {
        }
    }

    uart_send_string("MPU6050 initialise\r\n");

    /*
     * Calibration statique du gyroscope.
     *
     * Le MPU6050 doit rester complètement
     * immobile pendant cette phase.
     */
    uart_send_string("Calibration gyroscope...\r\n");
    uart_send_string("Ne pas bouger le MPU6050.\r\n");

    erreur = mpu6050_calibrate_gyro(&offset_gx,
                                &offset_gy,
                                &offset_gz);

    if (erreur != 0U)
    {
        uart_send_string("Erreur calibration gyroscope : ");
        uart_send_uint(erreur);
        uart_send_string("\r\n");

        while (1)
        {
        }
    }

    uart_send_string("Calibration terminee\r\n");

    uart_send_string("Offsets | GX = ");
    uart_send_int(offset_gx);

    uart_send_string(" | GY = ");
    uart_send_int(offset_gy);

    uart_send_string(" | GZ = ");
    uart_send_int(offset_gz);

    uart_send_string("\r\n\r\n");

    uart_send_string("Lecture IMU\r\n\r\n");

    while (1)
    {
        /*
         * Lecture de l'accéléromètre.
         */
        erreur = mpu6050_read_accel(&ax, &ay, &az);

        if (erreur != 0U)
        {
            uart_send_string("Erreur accelerometre : ");
            uart_send_uint(erreur);
            uart_send_string("\r\n");

            continue;
        }

        /*
         * Lecture du gyroscope.
         */
        erreur = mpu6050_read_gyro(&gx, &gy, &gz);

        if (erreur != 0U)
        {
            uart_send_string("Erreur gyroscope : ");
            uart_send_uint(erreur);
            uart_send_string("\r\n");

            continue;
        }

        /*
         * Compensation du biais statique
         * du gyroscope.
         */
        gx -= offset_gx;
        gy -= offset_gy;
        gz -= offset_gz;

        /*
         * Conversion de l'accéléromètre en g.
         */
        ax_g = mpu6050_accel_to_g(ax);
        ay_g = mpu6050_accel_to_g(ay);
        az_g = mpu6050_accel_to_g(az);

        /*
         * Conversion du gyroscope en degrés/s.
         */
        gx_dps = mpu6050_gyro_to_dps(gx);
        gy_dps = mpu6050_gyro_to_dps(gy);
        gz_dps = mpu6050_gyro_to_dps(gz);

        /*
         * Affichage UART.
         */
        afficher_imu(ax, ay, az,
                     gx, gy, gz,
                     ax_g, ay_g, az_g,
                     gx_dps, gy_dps, gz_dps);

        /*
         * Temporisation uniquement destinée
         * à rendre le terminal lisible.
         */
        delay_approx_ms(500);
    }
}