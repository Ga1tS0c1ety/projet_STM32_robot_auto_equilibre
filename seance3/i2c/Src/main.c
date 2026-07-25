#include "uart.h"
#include "i2c.h"

#include <stdint.h>

/*
 * Ces constantes appartiennent encore temporairement
 * au main.c.
 *
 * Elles seront déplacées dans mpu6050.h lors
 * de la deuxième passe de nettoyage.
 */
#define MPU6050_ADDRESS          0x68U
#define MPU6050_WHO_AM_I         0x75U
#define MPU6050_EXPECTED_ID      0x68U

int main(void)
{
    uint32_t resultat;
    uint8_t identifiant = 0;

    uart_init();

    uart_send_string("\r\nDemarrage du robot\r\n");
    uart_send_string("Initialisation I2C1...\r\n");

    i2c1_init();

    uart_send_string("I2C1 initialise : PB8/PB9, 100 kHz\r\n");

    /*
     * Premier test : vérifier que le MPU6050
     * répond à son adresse I2C.
     */
    uart_send_string("Test de l'adresse I2C 0x68...\r\n");

    resultat = i2c1_probe(MPU6050_ADDRESS);

    if (resultat == 0)
    {
        uart_send_string("MPU6050 detecte a l'adresse 0x68\r\n");
    }
    else
    {
        uart_send_string("Erreur pendant le test I2C, code = ");
        uart_send_uint(resultat);
        uart_send_string("\r\n");
    }

    /*
     * Deuxième test : lire le registre WHO_AM_I.
     */
    uart_send_string("Lecture du registre WHO_AM_I...\r\n");

    resultat = i2c1_read_register(
        MPU6050_ADDRESS,
        MPU6050_WHO_AM_I,
        &identifiant
    );

    if (resultat == 0)
    {
        uart_send_string("WHO_AM_I = ");
        uart_send_uint(identifiant);
        uart_send_string(" en decimal\r\n");

        if (identifiant == MPU6050_EXPECTED_ID)
        {
            uart_send_string("MPU6050 correctement identifie\r\n");
        }
        else
        {
            uart_send_string("Attention : identifiant inattendu\r\n");
        }
    }
    else
    {
        uart_send_string("Erreur de lecture I2C, code = ");
        uart_send_uint(resultat);
        uart_send_string("\r\n");
    }

    while (1)
    {
    }
}