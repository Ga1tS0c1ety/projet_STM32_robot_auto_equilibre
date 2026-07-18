#include <stdint.h>

/*
 * Séance 2 - UART TX
 * Carte : NUCLEO-F446RE
 * MCU   : STM32F446RE
 * UART  : USART2
 * TX    : PA2
 * Débit : 115200 bauds
 */

/* Adresses de base */
#define PERIPH_BASE        0x40000000UL //peripherique de base

#define APB1PERIPH_BASE    PERIPH_BASE //horloge pour le USART2
#define AHB1PERIPH_BASE    (PERIPH_BASE + 0x00020000UL) //horloge pour le GPIOA

#define GPIOA_BASE         (AHB1PERIPH_BASE + 0x0000UL) //Adresse de depart des GPIO
#define RCC_BASE           (AHB1PERIPH_BASE + 0x3800UL) //Adresse de depart de l'horloge du GPIO

#define USART2_BASE        (APB1PERIPH_BASE + 0x4400UL) //Adresse de depart des USART2

/* Registres RCC */
#define RCC_AHB1ENR        (*(volatile uint32_t *)(RCC_BASE + 0x30UL)) //Registre RCC_AHB1_ENABLE (horloge du GPIO)
#define RCC_APB1ENR        (*(volatile uint32_t *)(RCC_BASE + 0x40UL)) //Registre RCC_APB1_ENABLE (horloge du USART2)

/* Registres GPIOA */
#define GPIOA_MODER        (*(volatile uint32_t *)(GPIOA_BASE + 0x00UL)) //Registre du mode de fonctionnement des elements du GPIO
#define GPIOA_AFRL         (*(volatile uint32_t *)(GPIOA_BASE + 0x20UL)) //BUS de connection PA2 -> USART2

/* Registres USART2 */
#define USART2_SR          (*(volatile uint32_t *)(USART2_BASE + 0x00UL)) //Registre de verification de signal
#define USART2_DR          (*(volatile uint32_t *)(USART2_BASE + 0x04UL)) //Data register qui envoi les données
#define USART2_BRR         (*(volatile uint32_t *)(USART2_BASE + 0x08UL)) // Baud rate register qui definit le Baud Rate
#define USART2_CR1         (*(volatile uint32_t *)(USART2_BASE + 0x0CUL)) // Registre activation du mode Emetteur
#define USART2_CR2         (*(volatile uint32_t *)(USART2_BASE + 0x10UL)) // pas utilisé mais utile pour mettre à zero par precaution
#define USART2_CR3         (*(volatile uint32_t *)(USART2_BASE + 0x14UL)) // pas utilisé mais utile pour mettre à zero par precaution

/* Bits RCC */
#define RCC_AHB1ENR_GPIOAEN    (1UL << 0) //bit pour activer l'horloge du GPIO
#define RCC_APB1ENR_USART2EN   (1UL << 17) // bit pour activer l'horloge du USART2

/* Bits USART */
#define USART_SR_TXE           (1UL << 7) //bit activation de la disponibilité du transmetteur
#define USART_CR1_TE           (1UL << 3) //bit activation du transmetteurs
#define USART_CR1_UE           (1UL << 13) //bit activation de USART

static void uart_init(void)
{
    /* 1. Activer l’horloge de GPIOA */
    RCC_AHB1ENR |= RCC_AHB1ENR_GPIOAEN;

    /* 2. Activer l’horloge de USART2 */
    RCC_APB1ENR |= RCC_APB1ENR_USART2EN;

    /*
     * 3. Configurer PA2 en fonction alternative.
     *
     * PA2 utilise les bits MODER[5:4].
     * 10 = mode fonction alternative.
     */
    GPIOA_MODER &= ~(3UL << 4);
    GPIOA_MODER |=  (2UL << 4);

    /*
     * 4. Sélectionner AF7 sur PA2.
     *
     * PA2 utilise les bits AFRL[11:8].
     * AF7 correspond à USART2_TX.
     */
    GPIOA_AFRL &= ~(0xFUL << 8);
    GPIOA_AFRL |=  (7UL << 8);

    /*
     * 5. Remettre la configuration USART dans un état connu.
     *
     * Configuration obtenue :
     * - 8 bits de données ;
     * - aucune parité ;
     * - 1 bit de stop ;
     * - suréchantillonnage par 16.
     */
    USART2_CR1 = 0;
    USART2_CR2 = 0; //juste pour se rassurer
    USART2_CR3 = 0; //juste pour se rassurer

    /*
     * 6. Configurer le débit.
     *
     * Horloge USART2 : 16 MHz
     * Débit demandé  : 115200 bauds
     *
     * BRR ≈ 16 000 000 / 115 200 ≈ 139 = 0x8B
     */
    USART2_BRR = 0x8B;

    /* 7. Activer l’émetteur */
    USART2_CR1 |= USART_CR1_TE;

    /* 8. Activer USART2 */
    USART2_CR1 |= USART_CR1_UE;
}

static void uart_send_char(char caractere)
{
    /* Attendre que le registre de transmission soit disponible */
    while (!(USART2_SR & USART_SR_TXE))
    {
    }

    /* Envoyer le caractère */
    USART2_DR = (uint8_t)caractere;
}

static void uart_send_string(const char *texte)
{
    while (*texte != '\0')
    {
        uart_send_char(*texte);
        texte++;
    }
}

static void delay(uint32_t compteur)
{
    while (compteur--)
    {
        __asm volatile ("nop");
    }
}

static void uart_send_uint(uint32_t nombre)
{
    char chiffres[10];
    uint32_t position = 0;

    /*
     * Cas particulier :
     * la boucle suivante ne s'exécuterait pas pour zéro.
     */
    if (nombre == 0)
    {
        uart_send_char('0');
        return;
    }

    /*
     * Extraire les chiffres de droite à gauche.
     *
     * Exemple avec 123 :
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
     * Les chiffres sont enregistrés à l'envers.
     * Il faut donc les envoyer en partant de la fin.
     */
    while (position > 0)
    {
        position--;
        uart_send_char(chiffres[position]);
    }
}

int main(void)
{
    uart_init();
    uint32_t compteur = 1;

    while (1)
    {
        uart_send_string("Robot en fonctionnement : ");
        uart_send_uint(compteur);
        uart_send_string("\r\n");
        compteur+=0;
        delay(2000000);
    }
}