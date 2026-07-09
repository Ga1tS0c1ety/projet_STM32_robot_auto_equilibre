#include <stdint.h>

/*
 * Séance 1 - GPIO Blink
 * Carte : NUCLEO-F446RE
 * MCU   : STM32F446RE
 * LED   : LD2 sur PA5
 */

/* Adresses de base */
#define PERIPH_BASE        0x40000000UL
#define AHB1PERIPH_BASE    (PERIPH_BASE + 0x00020000UL)

#define GPIOA_BASE         (AHB1PERIPH_BASE + 0x0000UL)
#define RCC_BASE           (AHB1PERIPH_BASE + 0x3800UL)

/* Registres */
#define RCC_AHB1ENR        (*(volatile uint32_t *)(RCC_BASE + 0x30UL))
#define GPIOA_MODER        (*(volatile uint32_t *)(GPIOA_BASE + 0x00UL))
#define GPIOA_ODR          (*(volatile uint32_t *)(GPIOA_BASE + 0x14UL))

/* Bits utiles */
#define RCC_AHB1ENR_GPIOAEN    (1UL << 0)
#define LED_PIN                5U

static void delay(volatile uint32_t count)
{
    while (count--)
    {
        __asm volatile ("nop");
    }
}

int main(void)
{
    /* 1. Activer l'horloge du GPIOA */
    RCC_AHB1ENR |= RCC_AHB1ENR_GPIOAEN;

    /* 2. Configurer PA5 en sortie */
    GPIOA_MODER &= ~(3UL << (LED_PIN * 2U));
    GPIOA_MODER |=  (1UL << (LED_PIN * 2U));

    while (1)
    {
        /* 3. Inverser l'état de PA5 */
        GPIOA_ODR ^= (1UL << LED_PIN);

        /* 4. Attendre un peu */
        delay(400000);
    }
}