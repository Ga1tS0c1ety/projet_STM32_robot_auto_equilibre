#include <stdint.h>

#include "uart.h"
#include "i2c.h"
#include "mpu6050.h"

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

int main(void)
{
    uint32_t erreur;

    int16_t ax;
    int16_t ay;
    int16_t az;

    int16_t gx;
    int16_t gy;
    int16_t gz;

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
    uart_send_string("Lecture IMU brute\r\n\r\n");

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
         * Affichage des mesures brutes.
         */
        uart_send_string("ACC | X = ");
        uart_send_int(ax);

        uart_send_string(" | Y = ");
        uart_send_int(ay);

        uart_send_string(" | Z = ");
        uart_send_int(az);

        uart_send_string("\r\n");

        uart_send_string("GYR | X = ");
        uart_send_int(gx);

        uart_send_string(" | Y = ");
        uart_send_int(gy);

        uart_send_string(" | Z = ");
        uart_send_int(gz);

        uart_send_string("\r\n\r\n");

        /*
         * Temporisation uniquement pour
         * rendre l'affichage lisible.
         */
        delay_approx_ms(500);
    }
}