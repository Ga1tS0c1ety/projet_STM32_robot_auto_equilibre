#include <stdint.h>

#include "uart.h"
#include "pwm.h"

/*
 * Temporisation approximative.
 *
 * Elle est suffisante pour observer le test au multimètre,
 * mais elle ne constitue pas une base de temps précise.
 */
static void delay_approx_ms(uint32_t milliseconds)
{
    for (uint32_t ms = 0; ms < milliseconds; ms++)
    {
        for (volatile uint32_t compteur = 0; compteur < 4000U; compteur++)
        {
        }
    }
}

static void afficher_duty(uint32_t duty)
{
    uart_send_string("Rapport cyclique : ");
    uart_send_uint(duty);
    uart_send_string(" %\r\n");
}

int main(void)
{
    uart_init();

    uart_send_string("Demarrage du robot\r\n");
    uart_send_string("Initialisation du PWM...\r\n");

    pwm_init();

    uart_send_string("PWM TIM3_CH1 actif sur PA6\r\n");
    uart_send_string("Frequence : 1000 Hz\r\n");
    uart_send_string("Demarrage du test automatique\r\n");

    while (1)
    {
        /* Montée : 0 % vers 100 % */
        for (uint32_t duty = 0U; duty <= 100U; duty += 10U)
        {
            pwm_set_duty((uint8_t)duty);
            afficher_duty(duty);

            delay_approx_ms(1000U);
        }

        /*
         * Descente : 90 % vers 0 %.
         *
         * On commence à 90 % pour éviter d'afficher deux fois 100 %.
         * La variable est signée afin d'éviter un débordement sous zéro.
         */
        for (int32_t duty = 90; duty >= 0; duty -= 10)
        {
            pwm_set_duty((uint8_t)duty);
            afficher_duty((uint32_t)duty);

            delay_approx_ms(1000U);
        }
    }
}