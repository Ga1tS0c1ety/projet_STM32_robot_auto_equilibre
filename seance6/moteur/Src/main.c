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

static void test_pwm(uint32_t duty)
{
    moteur_avant();
    pwm_set_duty(duty);

    uart_send_string("PWM : ");
    uart_send_uint(duty);
    uart_send_string(" %\r\n");

    delay_approx_ms(3000);

    moteur_arreter();
    pwm_set_duty(0);

    uart_send_string("Arret\r\n\r\n");
    delay_approx_ms(1000);
}

int main(void)
{
    uart_init();
    pwm_init();
    moteur_init();

    uart_send_string("Demarrage du robot\r\n");
    uart_send_string("Test progressif PWM moteur\r\n\r\n");

    while (1)
    {
        test_pwm(20);
        test_pwm(40);
        test_pwm(60);
        test_pwm(80);

        uart_send_string("Fin du cycle\r\n");
        uart_send_string("--------------------\r\n");

        delay_approx_ms(3000);
    }
}