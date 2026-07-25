#include "i2c.h"

/*
 * Pilote I2C1 bare-metal
 *
 * Carte : NUCLEO-F446RE
 * MCU   : STM32F446RE
 * I2C   : I2C1
 * SCL   : PB8
 * SDA   : PB9
 * Débit : 100 kHz
 */

/* RCC */
#define RCC_BASE           0x40023800UL
#define RCC_AHB1ENR        (*(volatile uint32_t *)(RCC_BASE + 0x30UL))
#define RCC_APB1ENR        (*(volatile uint32_t *)(RCC_BASE + 0x40UL))

/* GPIOB */
#define GPIOB_BASE         0x40020400UL

#define GPIOB_MODER        (*(volatile uint32_t *)(GPIOB_BASE + 0x00UL))
#define GPIOB_OTYPER       (*(volatile uint32_t *)(GPIOB_BASE + 0x04UL))
#define GPIOB_OSPEEDR      (*(volatile uint32_t *)(GPIOB_BASE + 0x08UL))
#define GPIOB_PUPDR        (*(volatile uint32_t *)(GPIOB_BASE + 0x0CUL))
#define GPIOB_AFRH         (*(volatile uint32_t *)(GPIOB_BASE + 0x24UL))

/* I2C1 */
#define I2C1_BASE          0x40005400UL

#define I2C1_CR1           (*(volatile uint32_t *)(I2C1_BASE + 0x00UL))
#define I2C1_CR2           (*(volatile uint32_t *)(I2C1_BASE + 0x04UL))
#define I2C1_DR            (*(volatile uint32_t *)(I2C1_BASE + 0x10UL))
#define I2C1_SR1           (*(volatile uint32_t *)(I2C1_BASE + 0x14UL))
#define I2C1_SR2           (*(volatile uint32_t *)(I2C1_BASE + 0x18UL))
#define I2C1_CCR           (*(volatile uint32_t *)(I2C1_BASE + 0x1CUL))
#define I2C1_TRISE         (*(volatile uint32_t *)(I2C1_BASE + 0x20UL))

/* Bits RCC */
#define RCC_AHB1ENR_GPIOBEN    (1UL << 1)
#define RCC_APB1ENR_I2C1EN     (1UL << 21)

/* Bits I2C_CR1 */
#define I2C_CR1_PE         (1UL << 0)
#define I2C_CR1_START      (1UL << 8)
#define I2C_CR1_STOP       (1UL << 9)
#define I2C_CR1_ACK        (1UL << 10)

/* Bits I2C_SR1 */
#define I2C_SR1_SB         (1UL << 0)
#define I2C_SR1_ADDR       (1UL << 1)
#define I2C_SR1_BTF        (1UL << 2)
#define I2C_SR1_RXNE       (1UL << 6)
#define I2C_SR1_TXE        (1UL << 7)
#define I2C_SR1_AF         (1UL << 10)

/* Bits I2C_SR2 */
#define I2C_SR2_BUSY       (1UL << 1)

#define I2C_TIMEOUT        1000000UL

static void i2c1_configure_input_clock(void)
{
    /*
     * CR2[5:0] contient la fréquence de l'horloge APB1,
     * exprimée en MHz.
     *
     * PCLK1 = 16 MHz
     * FREQ  = 16
     */

    I2C1_CR2 &= ~0x3FUL;
    I2C1_CR2 |= 16UL;
}

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

static void i2c1_clock_enable(void)
{
    /*
     * Activer l’horloge du périphérique I2C1.
     *
     * I2C1EN correspond au bit 21 de RCC_APB1ENR.
     */
    RCC_APB1ENR |= RCC_APB1ENR_I2C1EN;

    /*
     * Lecture de synchronisation pour laisser
     * l’activation de l’horloge se propager.
     */
    (void)RCC_APB1ENR;
}

static void i2c1_configure_bus_speed(void)
{
    /*
     * Mode standard :
     * FS = 0
     *
     * PCLK1 = 16 MHz
     * SCL   = 100 kHz
     *
     * CCR = 16 000 000 / (2 × 100 000)
     * CCR = 80
     */

    I2C1_CCR = 80UL;
}

static void i2c1_configure_rise_time(void)
{
    /*
     * Mode standard à 100 kHz.
     *
     * PCLK1 = 16 MHz
     * TRISE = 16 + 1 = 17
     */
    I2C1_TRISE = 17UL;
}

static void i2c1_enable(void)
{
    /*
     * PE = Peripheral Enable.
     *
     * PE = 1 : périphérique I2C1 activé.
     * PE = 0 : périphérique I2C1 désactivé.
     */
    I2C1_CR1 |= I2C_CR1_PE;
}

uint32_t i2c1_probe(uint8_t adresse_7_bits)
{
    uint32_t timeout;

    /*
     * 1. Vérifier que le bus est libre.
     */
    timeout = I2C_TIMEOUT;

    while ((I2C1_SR2 & I2C_SR2_BUSY) && (timeout > 0))
    {
        timeout--;
    }

    if (timeout == 0)
    {
        return 1;
    }

    /*
     * 2. Demander une condition START.
     */
    I2C1_CR1 |= I2C_CR1_START;

    /*
     * 3. Attendre que START soit généré.
     *
     * SB = Start Bit.
     */
    timeout = I2C_TIMEOUT;

    while (!(I2C1_SR1 & I2C_SR1_SB) && (timeout > 0))
    {
        timeout--;
    }

    if (timeout == 0)
    {
        return 2;
    }

    /*
     * 4. Envoyer l'adresse du périphérique.
     *
     * Décalage à gauche :
     * - bits [7:1] : adresse ;
     * - bit  [0]   : R/W = 0 pour écriture.
     */
    I2C1_DR = ((uint32_t)adresse_7_bits << 1);

    /*
     * 5. Attendre soit :
     * - ADDR : le périphérique répond ;
     * - AF   : le périphérique ne répond pas.
     */
    timeout = I2C_TIMEOUT;

    while (!(I2C1_SR1 & (I2C_SR1_ADDR | I2C_SR1_AF))
           && (timeout > 0))
    {
        timeout--;
    }

    if (timeout == 0)
    {
        I2C1_CR1 |= I2C_CR1_STOP;
        return 3;
    }

    /*
     * AF = Acknowledge Failure.
     * Le périphérique n'a pas envoyé d'ACK.
     */
    if (I2C1_SR1 & I2C_SR1_AF)
    {
        I2C1_CR1 |= I2C_CR1_STOP;

        /* Effacer le drapeau AF. */
        I2C1_SR1 &= ~I2C_SR1_AF;

        return 4;
    }

    /*
     * Le périphérique a répondu.
     *
     * La lecture successive de SR1 puis SR2
     * efface le drapeau ADDR.
     */
    (void)I2C1_SR1;
    (void)I2C1_SR2;

    /*
     * Nous terminons ce simple test.
     */
    I2C1_CR1 |= I2C_CR1_STOP;

    return 0;
}

uint32_t i2c1_read_register(uint8_t adresse_7_bits,
                                   uint8_t registre,
                                   uint8_t *valeur)
{
    uint32_t timeout;

    /*
     * 1. Attendre que le bus soit libre.
     */
    timeout = I2C_TIMEOUT;

    while ((I2C1_SR2 & I2C_SR2_BUSY) && (timeout > 0))
    {
        timeout--;
    }

    if (timeout == 0)
    {
        return 1;
    }

    /*
     * 2. Générer START.
     */
    I2C1_CR1 |= I2C_CR1_START;

    timeout = I2C_TIMEOUT;

    while (!(I2C1_SR1 & I2C_SR1_SB) && (timeout > 0))
    {
        timeout--;
    }

    if (timeout == 0)
    {
        return 2;
    }

    /*
     * 3. Envoyer l’adresse du MPU6050 en écriture.
     *
     * Bit R/W = 0.
     */
    I2C1_DR = ((uint32_t)adresse_7_bits << 1);

    timeout = I2C_TIMEOUT;

    while (!(I2C1_SR1 & (I2C_SR1_ADDR | I2C_SR1_AF))
           && (timeout > 0))
    {
        timeout--;
    }

    if (timeout == 0)
    {
        I2C1_CR1 |= I2C_CR1_STOP;
        return 3;
    }

    if (I2C1_SR1 & I2C_SR1_AF)
    {
        I2C1_SR1 &= ~I2C_SR1_AF;
        I2C1_CR1 |= I2C_CR1_STOP;
        return 4;
    }

    /*
     * Effacer ADDR par une lecture de SR1 puis SR2.
     */
    (void)I2C1_SR1;
    (void)I2C1_SR2;

    /*
     * 4. Attendre que DR soit prêt,
     * puis envoyer l’adresse interne du registre.
     */
    timeout = I2C_TIMEOUT;

    while (!(I2C1_SR1 & I2C_SR1_TXE) && (timeout > 0))
    {
        timeout--;
    }

    if (timeout == 0)
    {
        I2C1_CR1 |= I2C_CR1_STOP;
        return 5;
    }

    I2C1_DR = registre;

    /*
     * Attendre la transmission complète du registre.
     */
    timeout = I2C_TIMEOUT;

    while (!(I2C1_SR1 & I2C_SR1_BTF) && (timeout > 0))
    {
        timeout--;
    }

    if (timeout == 0)
    {
        I2C1_CR1 |= I2C_CR1_STOP;
        return 6;
    }

    /*
     * 5. Générer un REPEATED START.
     */
    I2C1_CR1 |= I2C_CR1_START;

    timeout = I2C_TIMEOUT;

    while (!(I2C1_SR1 & I2C_SR1_SB) && (timeout > 0))
    {
        timeout--;
    }

    if (timeout == 0)
    {
        I2C1_CR1 |= I2C_CR1_STOP;
        return 7;
    }

    /*
     * 6. Envoyer l’adresse du MPU6050 en lecture.
     *
     * Bit R/W = 1.
     */
    I2C1_DR = ((uint32_t)adresse_7_bits << 1) | 1UL;

    timeout = I2C_TIMEOUT;

    while (!(I2C1_SR1 & (I2C_SR1_ADDR | I2C_SR1_AF))
           && (timeout > 0))
    {
        timeout--;
    }

    if (timeout == 0)
    {
        I2C1_CR1 |= I2C_CR1_STOP;
        return 8;
    }

    if (I2C1_SR1 & I2C_SR1_AF)
    {
        I2C1_SR1 &= ~I2C_SR1_AF;
        I2C1_CR1 |= I2C_CR1_STOP;
        return 9;
    }

    /*
     * 7. Réception d’un seul octet.
     *
     * ACK = 0 indique que nous ne voulons
     * recevoir qu’un seul octet.
     */
    I2C1_CR1 &= ~I2C_CR1_ACK;

    /*
     * Effacer ADDR en lisant SR1 puis SR2.
     */
    (void)I2C1_SR1;
    (void)I2C1_SR2;

    /*
     * Demander STOP avant la réception finale.
     */
    I2C1_CR1 |= I2C_CR1_STOP;

    /*
     * 8. Attendre l’octet reçu.
     */
    timeout = I2C_TIMEOUT;

    while (!(I2C1_SR1 & I2C_SR1_RXNE) && (timeout > 0))
    {
        timeout--;
    }

    if (timeout == 0)
    {
        return 10;
    }

    /*
     * Lire la valeur reçue.
     */
    *valeur = (uint8_t)I2C1_DR;

    /*
     * Réactiver ACK pour les futures opérations.
     */
    I2C1_CR1 |= I2C_CR1_ACK;

    return 0;
}

void i2c1_init(void)
{
    /*
     * 1. Préparer PB8 et PB9.
     */
    i2c1_gpio_init();

    /*
     * 2. Fournir une horloge à I2C1.
     */
    i2c1_clock_enable();

    /*
     * 3. Désactiver temporairement I2C1
     * pendant la configuration.
     */
    I2C1_CR1 &= ~I2C_CR1_PE;

    /*
     * 4. Configurer les temporisations.
     */
    i2c1_configure_input_clock();
    i2c1_configure_bus_speed();
    i2c1_configure_rise_time();

    /*
     * 5. Préparer les acquittements pour
     * les futures réceptions.
     */
    I2C1_CR1 |= I2C_CR1_ACK;

    /*
     * 6. Activer I2C1.
     */
    i2c1_enable();
}