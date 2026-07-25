#include "uart.h"
#include <stdint.h>

/* RCC */
#define RCC_BASE           0x40023800UL
#define RCC_AHB1ENR        (*(volatile uint32_t *)(RCC_BASE + 0x30UL))

/* GPIOB */
#define GPIOB_BASE         0x40020400UL

#define GPIOB_MODER        (*(volatile uint32_t *)(GPIOB_BASE + 0x00UL))
#define GPIOB_OTYPER       (*(volatile uint32_t *)(GPIOB_BASE + 0x04UL))
#define GPIOB_OSPEEDR      (*(volatile uint32_t *)(GPIOB_BASE + 0x08UL))
#define GPIOB_PUPDR        (*(volatile uint32_t *)(GPIOB_BASE + 0x0CUL))
#define GPIOB_AFRH         (*(volatile uint32_t *)(GPIOB_BASE + 0x24UL))

static void i2c1_gpio_init(void)
{
    /* 1. Activer l'horloge du port GPIOB */
    RCC_AHB1ENR |= (1UL << 1);

    /*
     * Petite lecture de synchronisation.
     * Elle laisse le temps à l'horloge GPIOB de devenir active.
     */
    (void)RCC_AHB1ENR;

    /*
     * 2. PB8 et PB9 en mode fonction alternative
     *
     * MODER8 = 10
     * MODER9 = 10
     */
    GPIOB_MODER &= ~((3UL << 16) | (3UL << 18));
    GPIOB_MODER |=  ((2UL << 16) | (2UL << 18));

    /*
     * 3. Sorties open-drain
     *
     * OT8 = 1
     * OT9 = 1
     */
    GPIOB_OTYPER |= (1UL << 8) | (1UL << 9);

    /*
     * 4. Vitesse élevée
     *
     * OSPEEDR8 = 11
     * OSPEEDR9 = 11
     */
    GPIOB_OSPEEDR &= ~((3UL << 16) | (3UL << 18));
    GPIOB_OSPEEDR |=  ((3UL << 16) | (3UL << 18));

    /*
     * 5. Résistances pull-up internes
     *
     * PUPDR8 = 01
     * PUPDR9 = 01
     */
    GPIOB_PUPDR &= ~((3UL << 16) | (3UL << 18));
    GPIOB_PUPDR |=  ((1UL << 16) | (1UL << 18));

    /*
     * 6. Fonction alternative AF4
     *
     * PB8 utilise AFRH[3:0]
     * PB9 utilise AFRH[7:4]
     */
    GPIOB_AFRH &= ~((0xFUL << 0) | (0xFUL << 4));
    GPIOB_AFRH |=  ((4UL << 0) | (4UL << 4));
}

int main(void)
{
    uart_init();

    uart_send_string("Demarrage du robot\r\n");
    uart_send_string("Configuration des broches I2C...\r\n");

    i2c1_gpio_init();

    uart_send_string("PB8 et PB9 configurees en AF4\r\n");

    while (1)
    {
    }
}