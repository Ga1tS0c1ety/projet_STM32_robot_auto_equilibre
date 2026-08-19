#include <stdint.h>

#include "moteur.h"

/*
 * Adresses de base
 */
#define PERIPH_BASE        0x40000000UL
#define AHB1PERIPH_BASE    (PERIPH_BASE + 0x00020000UL)

#define GPIOC_BASE         (AHB1PERIPH_BASE + 0x0800UL)
#define RCC_BASE           (AHB1PERIPH_BASE + 0x3800UL)

/*
 * Registres RCC
 */
#define RCC_AHB1ENR        (*(volatile uint32_t *)(RCC_BASE + 0x30UL))

/*
 * Registres GPIOC
 */
#define GPIOC_MODER        (*(volatile uint32_t *)(GPIOC_BASE + 0x00UL))
#define GPIOC_OTYPER       (*(volatile uint32_t *)(GPIOC_BASE + 0x04UL))
#define GPIOC_PUPDR        (*(volatile uint32_t *)(GPIOC_BASE + 0x0CUL))
#define GPIOC_ODR          (*(volatile uint32_t *)(GPIOC_BASE + 0x14UL))

/*
 * Broches utilisées
 *
 * PC0 -> AIN1
 * PC1 -> AIN2
 * PC2 -> STBY
 */
#define MOTEUR_AIN1        0U
#define MOTEUR_AIN2        1U
#define MOTEUR_STBY        2U

void moteur_init(void)
{
    /*
     * Activation de l'horloge GPIOC.
     * GPIOCEN = bit 2 de RCC_AHB1ENR.
     */
    RCC_AHB1ENR |= (1UL << 2);

    /*
     * PC0, PC1 et PC2 en mode sortie.
     *
     * MODER :
     * 00 = Input
     * 01 = Output
     * 10 = Alternate Function
     * 11 = Analog
     */

    GPIOC_MODER &= ~((3UL << (MOTEUR_AIN1 * 2U)) |
                     (3UL << (MOTEUR_AIN2 * 2U)) |
                     (3UL << (MOTEUR_STBY * 2U)));

    GPIOC_MODER |=  ((1UL << (MOTEUR_AIN1 * 2U)) |
                     (1UL << (MOTEUR_AIN2 * 2U)) |
                     (1UL << (MOTEUR_STBY * 2U)));

    /*
     * Sorties push-pull.
     */
    GPIOC_OTYPER &= ~((1UL << MOTEUR_AIN1) |
                      (1UL << MOTEUR_AIN2) |
                      (1UL << MOTEUR_STBY));

    /*
     * Pas de pull-up / pull-down.
     */
    GPIOC_PUPDR &= ~((3UL << (MOTEUR_AIN1 * 2U)) |
                     (3UL << (MOTEUR_AIN2 * 2U)) |
                     (3UL << (MOTEUR_STBY * 2U)));

    /*
     * Etat initial :
     * moteur arrêté.
     */
    GPIOC_ODR &= ~((1UL << MOTEUR_AIN1) |
                   (1UL << MOTEUR_AIN2));

    /*
     * Activation du TB6612FNG.
     */
    moteur_activer();
}

void moteur_activer(void)
{
    GPIOC_ODR |= (1UL << MOTEUR_STBY);
}

void moteur_desactiver(void)
{
    GPIOC_ODR &= ~(1UL << MOTEUR_STBY);
}

void moteur_avant(void)
{
    GPIOC_ODR |=  (1UL << MOTEUR_AIN1);
    GPIOC_ODR &= ~(1UL << MOTEUR_AIN2);
}

void moteur_arriere(void)
{
    GPIOC_ODR &= ~(1UL << MOTEUR_AIN1);
    GPIOC_ODR |=  (1UL << MOTEUR_AIN2);
}

void moteur_arreter(void)
{
    GPIOC_ODR &= ~((1UL << MOTEUR_AIN1) |
                   (1UL << MOTEUR_AIN2));
}