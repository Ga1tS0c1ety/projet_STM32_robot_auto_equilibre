#ifndef I2C_H
#define I2C_H

#include <stdint.h>

/*
 * Initialise I2C1 :
 * - SCL sur PB8 ;
 * - SDA sur PB9 ;
 * - mode standard ;
 * - fréquence du bus : 100 kHz ;
 * - horloge APB1 : 16 MHz.
 */
void i2c1_init(void);

/*
 * Teste si un périphérique répond à une adresse I2C.
 *
 * Retour :
 * 0 : périphérique détecté ;
 * autre valeur : erreur.
 */
uint32_t i2c1_probe(uint8_t adresse_7_bits);

/*
 * Lit un registre de 8 bits dans un périphérique I2C.
 *
 * Retour :
 * 0 : lecture réussie ;
 * autre valeur : erreur.
 */
uint32_t i2c1_read_register(uint8_t adresse_7_bits,
                            uint8_t registre,
                            uint8_t *valeur);

#endif /* I2C_H */