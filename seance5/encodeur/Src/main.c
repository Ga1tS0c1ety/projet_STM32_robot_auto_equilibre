#include <stdint.h>

#include "uart.h"
#include "encoder.h"
#include "timebase.h"

#define SPEED_SAMPLE_PERIOD_US 100000UL


static void afficher_int32(int32_t valeur)
{
    if (valeur < 0)
    {
        uart_send_string("-");
        uart_send_uint((uint32_t)(-(int64_t)valeur));
    }
    else
    {
        uart_send_uint((uint32_t)valeur);
    }
}


int main(void)
{
    int32_t position_precedente;
    uint32_t temps_precedent;

    uart_init();
    encoder_init();
    timebase_init();

    uart_send_string("Demarrage du robot\r\n");
    uart_send_string("Encodeur TIM2 actif\r\n");
    uart_send_string("Base de temps TIM5 active\r\n");
    uart_send_string("Mesure vitesse toutes les 100 ms\r\n");

    position_precedente = encoder_get_count();
    temps_precedent = timebase_get_us();

    while (1)
    {
        uint32_t temps_actuel = timebase_get_us();
        uint32_t delta_us = temps_actuel - temps_precedent;

        if (delta_us >= SPEED_SAMPLE_PERIOD_US)
        {
            int32_t position_actuelle = encoder_get_count();

            int32_t delta_count =
                position_actuelle - position_precedente;

            int32_t rpm =
                encoder_compute_rpm(delta_count, delta_us);

            uart_send_string("Position : ");
            afficher_int32(position_actuelle);

            uart_send_string(" | Delta : ");
            afficher_int32(delta_count);

            uart_send_string(" | Vitesse : ");
            afficher_int32((int32_t)rpm);

            uart_send_string(" tr/min\r\n");

            position_precedente = position_actuelle;
            temps_precedent = temps_actuel;
        }
    }
}