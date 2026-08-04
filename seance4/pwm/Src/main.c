#include <stdint.h>

#include "uart.h"
#include "pwm.h"

int main(void)
{
    uint8_t duty = 100U;

    uart_init();

    uart_send_string("Demarrage du robot\r\n");
    uart_send_string("Initialisation du PWM...\r\n");

    pwm_init();
    pwm_set_duty(duty);

    uart_send_string("PWM TIM3_CH1 actif sur PA6\r\n");
    uart_send_string("Frequence : 1000 Hz\r\n");

    uart_send_string("Rapport cyclique : ");
    uart_send_uint(duty);
    uart_send_string(" %\r\n");

    while (1)
    {
    }
}