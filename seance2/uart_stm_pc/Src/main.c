#include <stdint.h>

#include "uart.h"

static void delay(uint32_t compteur)
{
    while (compteur--)
    {
        __asm volatile ("nop");
    }
}

int main(void)
{
    uint32_t compteur = 1;

    uart_init();

    uart_send_string("Demarrage du robot\r\n");

    while (1)
    {
        uart_send_string("Message numero ");
        uart_send_uint(compteur);
        uart_send_string("\r\n");

        compteur++;

        delay(1000000);
    }
}