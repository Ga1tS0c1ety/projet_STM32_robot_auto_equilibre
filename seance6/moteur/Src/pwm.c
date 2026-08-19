#include "pwm.h"

/* =========================================================
 * Adresses de base
 * ========================================================= */

#define PERIPH_BASE        0x40000000UL

#define APB1PERIPH_BASE    (PERIPH_BASE + 0x00000000UL)
#define AHB1PERIPH_BASE    (PERIPH_BASE + 0x00020000UL)

#define TIM3_BASE          (APB1PERIPH_BASE + 0x0400UL)
#define GPIOA_BASE         (AHB1PERIPH_BASE + 0x0000UL)
#define RCC_BASE           (AHB1PERIPH_BASE + 0x3800UL)

/* =========================================================
 * Registres RCC
 * ========================================================= */

#define RCC_AHB1ENR        (*(volatile uint32_t *)(RCC_BASE + 0x30UL))
#define RCC_APB1ENR        (*(volatile uint32_t *)(RCC_BASE + 0x40UL))

/* =========================================================
 * Registres GPIOA
 * ========================================================= */

#define GPIOA_MODER        (*(volatile uint32_t *)(GPIOA_BASE + 0x00UL))
#define GPIOA_OTYPER       (*(volatile uint32_t *)(GPIOA_BASE + 0x04UL))
#define GPIOA_PUPDR        (*(volatile uint32_t *)(GPIOA_BASE + 0x0CUL))
#define GPIOA_AFRL         (*(volatile uint32_t *)(GPIOA_BASE + 0x20UL))

/* =========================================================
 * Registres TIM3
 * ========================================================= */

#define TIM3_CR1           (*(volatile uint32_t *)(TIM3_BASE + 0x00UL))
#define TIM3_EGR           (*(volatile uint32_t *)(TIM3_BASE + 0x14UL))
#define TIM3_CCMR1         (*(volatile uint32_t *)(TIM3_BASE + 0x18UL))
#define TIM3_CCER          (*(volatile uint32_t *)(TIM3_BASE + 0x20UL))
#define TIM3_CNT           (*(volatile uint32_t *)(TIM3_BASE + 0x24UL))
#define TIM3_PSC           (*(volatile uint32_t *)(TIM3_BASE + 0x28UL))
#define TIM3_ARR           (*(volatile uint32_t *)(TIM3_BASE + 0x2CUL))
#define TIM3_CCR1          (*(volatile uint32_t *)(TIM3_BASE + 0x34UL))

/* =========================================================
 * Bits utiles
 * ========================================================= */

#define RCC_AHB1ENR_GPIOAEN    (1UL << 0) 
#define RCC_APB1ENR_TIM3EN     (1UL << 1) //horloge du timer 

#define TIM_CR1_CEN            (1UL << 0) // Activation du compteur
#define TIM_CR1_ARPE           (1UL << 7) //preload de ARR

#define TIM_EGR_UG             (1UL << 0) // Chargement des parametres

#define TIM_CCMR1_OC1PE        (1UL << 3) // Preload du CCR1

#define TIM_CCER_CC1E          (1UL << 0) //activation du canal 
#define TIM_CCER_CC1P          (1UL << 1) // polarité de depart pour le CCR (ici à 1)

/* =========================================================
 * Initialisation du PWM
 * ========================================================= */

void pwm_init(void)
{
    /* Activer GPIOA et TIM3 */
    RCC_AHB1ENR |= RCC_AHB1ENR_GPIOAEN;
    RCC_APB1ENR |= RCC_APB1ENR_TIM3EN;

    /*
     * PA6 en fonction alternative.
     * MODER6 = 10.
     */
    GPIOA_MODER &= ~(3UL << (6U * 2U));
    GPIOA_MODER |=  (2UL << (6U * 2U));

    /* Sortie push-pull */
    GPIOA_OTYPER &= ~(1UL << 6);

    /* Aucun pull-up ou pull-down */
    GPIOA_PUPDR &= ~(3UL << (6U * 2U));

    /*
     * PA6 = AF2 = TIM3_CH1.
     * AFRL6 correspond aux bits [27:24].
     */
    GPIOA_AFRL &= ~(0xFUL << (6U * 4U));
    GPIOA_AFRL |=  (2UL   << (6U * 4U));

    /* Arrêter TIM3 pendant la configuration */
    TIM3_CR1 &= ~TIM_CR1_CEN;

    /* Remettre le compteur à zéro */
    TIM3_CNT = 0UL;

    /*
     * Horloge supposée de TIM3 : 16 MHz.
     *
     * 16 MHz / (15 + 1) = 1 MHz.
     */
    TIM3_PSC = 15UL;

    /*
     * 1 MHz / (999 + 1) = 1 kHz.
     */
    TIM3_ARR = 999UL;

    /* Démarrage sécurisé à 0 % */
    TIM3_CCR1 = 0UL;

    /*
     * OC1M = 110 : PWM mode 1.
     */
    TIM3_CCMR1 &= ~(7UL << 4);
    TIM3_CCMR1 |=  (6UL << 4);

    /* Activer le preload de CCR1 */
    TIM3_CCMR1 |= TIM_CCMR1_OC1PE;

    /* Polarité active à l'état haut */
    TIM3_CCER &= ~TIM_CCER_CC1P;

    /* Activer la sortie du canal 1 */
    TIM3_CCER |= TIM_CCER_CC1E;

    /* Activer le preload de ARR */
    TIM3_CR1 |= TIM_CR1_ARPE;

    /* Charger PSC, ARR et CCR1 */
    TIM3_EGR |= TIM_EGR_UG;

    /* Remettre le compteur à zéro après l'update */
    TIM3_CNT = 0UL;

    /* Démarrer TIM3 */
    TIM3_CR1 |= TIM_CR1_CEN;
}

/* =========================================================
 * Modification du rapport cyclique
 * ========================================================= */

void pwm_set_duty(uint8_t duty_percent)
{
    if (duty_percent > 100U)
    {
        duty_percent = 100U;
    }

    TIM3_CCR1 =
        ((TIM3_ARR + 1UL) * duty_percent) / 100UL;
}