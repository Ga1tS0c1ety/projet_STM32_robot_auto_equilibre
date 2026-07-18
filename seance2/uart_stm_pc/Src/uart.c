#include "uart.h"

/*
 * Séance 2 - Pilote UART bare-metal
 *
 * Carte : NUCLEO-F446RE
 * MCU   : STM32F446RE
 * UART  : USART2
 * TX    : PA2
 * Débit : 115200 bauds
 */

/* Adresses de base */
#define PERIPH_BASE        0x40000000UL

#define APB1PERIPH_BASE    PERIPH_BASE
#define AHB1PERIPH_BASE    (PERIPH_BASE + 0x00020000UL)

#define GPIOA_BASE         (AHB1PERIPH_BASE + 0x0000UL)
#define RCC_BASE           (AHB1PERIPH_BASE + 0x3800UL)
#define USART2_BASE        (APB1PERIPH_BASE + 0x4400UL)

/* Registres RCC */
#define RCC_AHB1ENR        (*(volatile uint32_t *)(RCC_BASE + 0x30UL))
#define RCC_APB1ENR        (*(volatile uint32_t *)(RCC_BASE + 0x40UL))

/* Registres GPIOA */
#define GPIOA_MODER        (*(volatile uint32_t *)(GPIOA_BASE + 0x00UL))
#define GPIOA_AFRL         (*(volatile uint32_t *)(GPIOA_BASE + 0x20UL))

/* Registres USART2 */
#define USART2_SR          (*(volatile uint32_t *)(USART2_BASE + 0x00UL))
#define USART2_DR          (*(volatile uint32_t *)(USART2_BASE + 0x04UL))
#define USART2_BRR         (*(volatile uint32_t *)(USART2_BASE + 0x08UL))
#define USART2_CR1         (*(volatile uint32_t *)(USART2_BASE + 0x0CUL))
#define USART2_CR2         (*(volatile uint32_t *)(USART2_BASE + 0x10UL))
#define USART2_CR3         (*(volatile uint32_t *)(USART2_BASE + 0x14UL))

/* Bits RCC */
#define RCC_AHB1ENR_GPIOAEN    (1UL << 0)
#define RCC_APB1ENR_USART2EN   (1UL << 17)

/* Bits USART2 */
#define USART_SR_TXE           (1UL << 7)
#define USART_CR1_TE           (1UL << 3)
#define USART_CR1_UE           (1UL << 13)

void uart_init(void)
{
    /* Activer l’horloge de GPIOA */
    RCC_AHB1ENR |= RCC_AHB1ENR_GPIOAEN;

    /* Activer l’horloge de USART2 */
    RCC_APB1ENR |= RCC_APB1ENR_USART2EN;

    /*
     * Configurer PA2 en fonction alternative.
     *
     * PA2 utilise MODER[5:4].
     * 10 = fonction alternative.
     */
    GPIOA_MODER &= ~(3UL << 4);
    GPIOA_MODER |=  (2UL << 4);

    /*
     * Sélectionner AF7 pour PA2.
     *
     * PA2 utilise AFRL[11:8].
     * AF7 permet de connecter PA2 à USART2_TX.
     */
    GPIOA_AFRL &= ~(0xFUL << 8);
    GPIOA_AFRL |=  (7UL << 8);

    /*
     * Remettre USART2 dans un état connu :
     * - 8 bits ;
     * - aucune parité ;
     * - 1 bit de stop ;
     * - suréchantillonnage par 16.
     */
    USART2_CR1 = 0;
    USART2_CR2 = 0;
    USART2_CR3 = 0;

    /*
     * Débit de 115200 bauds.
     *
     * Horloge USART2 : 16 MHz
     * BRR = 16 000 000 / 115 200 ≈ 139 = 0x8B
     */
    USART2_BRR = 0x8B;

    /* Activer l’émetteur USART */
    USART2_CR1 |= USART_CR1_TE;

    /* Activer USART2 */
    USART2_CR1 |= USART_CR1_UE;
}

void uart_send_char(char caractere)
{
    /*
     * Attendre que le registre de transmission soit vide.
     *
     * TXE = 1 signifie qu’un nouveau caractère peut être écrit.
     */
    while (!(USART2_SR & USART_SR_TXE))
    {
    }

    USART2_DR = (uint8_t)caractere;
}

void uart_send_string(const char *texte)
{
    /*
     * Parcourir la chaîne jusqu’au caractère terminal '\0'.
     */
    while (*texte != '\0')
    {
        uart_send_char(*texte);
        texte++;
    }
}

void uart_send_uint(uint32_t nombre)
{
    /*
     * Un uint32_t contient au maximum dix chiffres décimaux :
     * 4 294 967 295.
     */
    char chiffres[10];
    uint32_t position = 0;

    /* Cas particulier du nombre zéro */
    if (nombre == 0)
    {
        uart_send_char('0');
        return;
    }

    /*
     * Extraire les chiffres de droite à gauche.
     *
     * Exemple pour 123 :
     * 123 % 10 = 3
     * 12  % 10 = 2
     * 1   % 10 = 1
     */
    while (nombre > 0)
    {
        uint32_t chiffre = nombre % 10;

        chiffres[position] = (char)('0' + chiffre);
        position++;

        nombre /= 10;
    }

    /*
     * Les chiffres ont été stockés à l’envers.
     * On les transmet donc de la fin vers le début.
     */
    while (position > 0)
    {
        position--;
        uart_send_char(chiffres[position]);
    }
}