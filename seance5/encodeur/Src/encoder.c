#include <stdint.h>

#include "encoder.h"

/*
 * Adresses de base
 */
#define PERIPH_BASE        0x40000000UL
#define APB1PERIPH_BASE    PERIPH_BASE
#define AHB1PERIPH_BASE    (PERIPH_BASE + 0x00020000UL)

#define TIM2_BASE          (APB1PERIPH_BASE + 0x0000UL)
#define GPIOA_BASE         (AHB1PERIPH_BASE + 0x0000UL)
#define RCC_BASE           (AHB1PERIPH_BASE + 0x3800UL)

/*
 * Registres RCC
 */
#define RCC_AHB1ENR        (*(volatile uint32_t *)(RCC_BASE + 0x30UL))
#define RCC_APB1ENR        (*(volatile uint32_t *)(RCC_BASE + 0x40UL))

/*
 * Registres GPIOA
 */
#define GPIOA_MODER        (*(volatile uint32_t *)(GPIOA_BASE + 0x00UL))
#define GPIOA_AFRL         (*(volatile uint32_t *)(GPIOA_BASE + 0x20UL))

/*
 * Registres TIM2
 */
#define TIM2_CR1           (*(volatile uint32_t *)(TIM2_BASE + 0x00UL))
#define TIM2_SMCR          (*(volatile uint32_t *)(TIM2_BASE + 0x08UL))
#define TIM2_CCMR1         (*(volatile uint32_t *)(TIM2_BASE + 0x18UL))
#define TIM2_CCER          (*(volatile uint32_t *)(TIM2_BASE + 0x20UL))
#define TIM2_CNT           (*(volatile uint32_t *)(TIM2_BASE + 0x24UL))
#define TIM2_ARR           (*(volatile uint32_t *)(TIM2_BASE + 0x2CUL))


void encoder_init(void)
{
    /*
     * 1. Activer GPIOA.
     */
    RCC_AHB1ENR |= (1UL << 0);

    /*
     * 2. PA0 et PA1 en Alternate Function.
     *
     * PA0 MODER[1:0] = 10
     * PA1 MODER[3:2] = 10
     */
    GPIOA_MODER &= ~((3UL << 0) | (3UL << 2));
    GPIOA_MODER |=  ((2UL << 0) | (2UL << 2));

    /*
     * 3. Sélectionner AF1 pour PA0 et PA1.
     *
     * PA0 -> TIM2_CH1
     * PA1 -> TIM2_CH2
     */
    GPIOA_AFRL &= ~((0xFUL << 0) | (0xFUL << 4));
    GPIOA_AFRL |=  ((1UL   << 0) | (1UL   << 4));

    /*
     * 4. Activer TIM2.
     */
    RCC_APB1ENR |= (1UL << 0);

    /*
     * Arrêter TIM2 pendant sa configuration.
     */
    TIM2_CR1 &= ~(1UL << 0);

    /*
     * 5. Configurer CH1 et CH2 comme entrées.
     *
     * CC1S = 01 -> IC1 connecté à TI1
     * CC2S = 01 -> IC2 connecté à TI2
     */
    TIM2_CCMR1 &= ~((3UL << 0) | (3UL << 8));
    TIM2_CCMR1 |=  ((1UL << 0) | (1UL << 8));

    /*
     * 6. Polarité normale des deux entrées.
     *
     * CC1P = 0
     * CC2P = 0
     */
    TIM2_CCER &= ~((1UL << 1) | (1UL << 5));

    /*
     * 7. Encoder mode 3.
     *
     * SMS = 011
     * Le compteur utilise les fronts de TI1 ET TI2.
     */
    TIM2_SMCR &= ~(7UL << 0);
    TIM2_SMCR |=  (3UL << 0);

    /*
     * 8. TIM2 est un compteur 32 bits.
     */
    TIM2_ARR = 0xFFFFFFFFUL;

    /*
     * 9. Commencer à la position zéro.
     */
    TIM2_CNT = 0UL;

    /*
     * 10. Démarrer TIM2.
     */
    TIM2_CR1 |= (1UL << 0);
}


int32_t encoder_get_count(void)
{
    return (int32_t)TIM2_CNT;
}