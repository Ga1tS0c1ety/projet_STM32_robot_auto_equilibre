#include <stdint.h>

#include "uart.h"
#include "encoder.h"


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


static void afficher_position(int32_t position)
{
    uart_send_string("Position : ");

    if (position < 0)
    {
        uart_send_string("-");
        uart_send_uint((uint32_t)(-(int64_t)position));
    }
    else
    {
        uart_send_uint((uint32_t)position);
    }

    uart_send_string("\r\n");
}


int main(void)
{
    uart_init();

    uart_send_string("Demarrage du robot\r\n");
    uart_send_string("Initialisation encodeur...\r\n");

    encoder_init();

    uart_send_string("Encodeur TIM2 actif\r\n");
    uart_send_string("PA0 = voie A / PA1 = voie B\r\n");

    while (1)
    {
        afficher_position(encoder_get_count());

        delay_approx_ms(200);
    }
}