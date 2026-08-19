#include <stdint.h>

#include "uart.h"
#include "pwm.h"
#include "moteur.h"
#include "encoder.h"
#include "timebase.h"

static void attendre_ms(uint32_t ms)
{
    uint32_t debut = timebase_get_us();

    while ((uint32_t)(timebase_get_us() - debut) < (ms * 1000U))
    {
    }
}

static void uart_send_int32(int32_t valeur)
{
    if (valeur < 0)
    {
        uart_send_string("-");
        uart_send_uint((uint32_t)(-valeur));
    }
    else
    {
        uart_send_uint((uint32_t)valeur);
    }
}

static void test_pwm_mesure(uint32_t duty)
{
    int32_t position_precedente;
    int32_t position_actuelle;
    int32_t delta_count;
    int32_t rpm;

    uint32_t temps_precedent;
    uint32_t temps_actuel;
    uint32_t delta_us;
    uint32_t debut_test;

    uart_send_string("\r\nPWM : ");
    uart_send_int32(duty);
    uart_send_string(" %\r\n");

    /*
     * Mise en marche du moteur.
     */
    moteur_avant();
    pwm_set_duty(duty);

    /*
     * Valeurs initiales pour la mesure.
     */
    position_precedente = encoder_get_count();
    temps_precedent = timebase_get_us();

    debut_test = temps_precedent;

    /*
     * Test pendant 3 secondes.
     */
    while ((uint32_t)(timebase_get_us() - debut_test) < 3000000U)
    {
        temps_actuel = timebase_get_us();

        /*
         * Nouvelle mesure toutes les 100 ms.
         */
        if ((uint32_t)(temps_actuel - temps_precedent) >= 100000U)
        {
            position_actuelle = encoder_get_count();

            delta_count = position_actuelle - position_precedente;
            delta_us = temps_actuel - temps_precedent;

            rpm = encoder_compute_rpm(delta_count, delta_us);

            uart_send_string("Position : ");
            uart_send_int32(position_actuelle);

            uart_send_string(" | Delta : ");
            uart_send_int32(delta_count);

            uart_send_string(" | Vitesse : ");
            uart_send_int32(rpm);

            uart_send_string(" tr/min\r\n");

            /*
             * La mesure actuelle devient la référence
             * pour la prochaine période.
             */
            position_precedente = position_actuelle;
            temps_precedent = temps_actuel;
        }
    }

    /*
     * Arrêt entre deux essais.
     */
    moteur_arreter();
    pwm_set_duty(0);

    uart_send_string("Arret\r\n");
    attendre_ms(1000);
}

int main(void)
{
    uart_init();
    pwm_init();
    moteur_init();
    encoder_init();
    timebase_init();

    uart_send_string("Demarrage du robot\r\n");
    uart_send_string("Test PWM + Encodeur\r\n");

    while (1)
    {
        test_pwm_mesure(20);
        test_pwm_mesure(40);
        test_pwm_mesure(60);
        test_pwm_mesure(80);
    }
}