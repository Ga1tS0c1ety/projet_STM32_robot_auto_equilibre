#include <stdint.h>

#include "uart.h"
#include "pwm.h"
#include "moteur.h"

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
    uart_init();
    pwm_init();
    moteur_init();

    uart_send_string("Demarrage du robot\r\n");
    uart_send_string("Test direction moteur\r\n");

    while (1)
    {
        /* Avant */
        moteur_avant();
        pwm_set_duty(30);

        uart_send_string("Moteur : AVANT\r\n");
        delay_approx_ms(2000);

        /* Arret */
        moteur_arreter();
        pwm_set_duty(0);

        uart_send_string("Moteur : ARRET\r\n");
        delay_approx_ms(1000);

        /* Arriere */
        moteur_arriere();
        pwm_set_duty(30);

        uart_send_string("Moteur : ARRIERE\r\n");
        delay_approx_ms(2000);

        /* Arret */
        moteur_arreter();
        pwm_set_duty(0);

        uart_send_string("Moteur : ARRET\r\n");
        delay_approx_ms(1000);
    }
}