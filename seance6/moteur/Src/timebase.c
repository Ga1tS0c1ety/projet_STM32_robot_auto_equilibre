#include <stdint.h>

#include "timebase.h"

#define PERIPH_BASE        0x40000000UL
#define APB1PERIPH_BASE    PERIPH_BASE
#define AHB1PERIPH_BASE    (PERIPH_BASE + 0x00020000UL)

#define TIM5_BASE          (APB1PERIPH_BASE + 0x0C00UL)
#define RCC_BASE           (AHB1PERIPH_BASE + 0x3800UL)

#define RCC_APB1ENR        (*(volatile uint32_t *)(RCC_BASE + 0x40UL))

#define TIM5_CR1           (*(volatile uint32_t *)(TIM5_BASE + 0x00UL))
#define TIM5_EGR           (*(volatile uint32_t *)(TIM5_BASE + 0x14UL))
#define TIM5_CNT           (*(volatile uint32_t *)(TIM5_BASE + 0x24UL))
#define TIM5_PSC           (*(volatile uint32_t *)(TIM5_BASE + 0x28UL))
#define TIM5_ARR           (*(volatile uint32_t *)(TIM5_BASE + 0x2CUL))

void timebase_init(void)
{
    /*
     * TIM5 est sur APB1.
     * TIM5EN = bit 3.
     */
    RCC_APB1ENR |= (1UL << 3);

    /*
     * Arrêter le compteur pendant la configuration.
     */
    TIM5_CR1 &= ~(1UL << 0);

    /*
     * 16 MHz / (15 + 1) = 1 MHz
     * donc 1 tick = 1 us.
     */
    TIM5_PSC = 15UL;

    /*
     * TIM5 est un timer 32 bits.
     */
    TIM5_ARR = 0xFFFFFFFFUL;

    /*
     * Générer un Update Event pour charger PSC.
     */
    TIM5_EGR = (1UL << 0);

    /*
     * Commencer à zéro.
     */
    TIM5_CNT = 0UL;

    /*
     * CEN = 1 : démarrer TIM5.
     */
    TIM5_CR1 |= (1UL << 0);
}

uint32_t timebase_get_us(void)
{
    return TIM5_CNT;
}