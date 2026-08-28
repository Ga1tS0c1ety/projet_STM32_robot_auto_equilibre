#ifndef MPU6050_H
#define MPU6050_H

#include <stdint.h>

/*
 * Initialise le MPU6050.
 *
 * Retour :
 * 0 : succès ;
 * autre valeur : erreur I2C.
 */
uint32_t mpu6050_init(void);

/*
 * Lit les valeurs brutes de l'accéléromètre.
 */
uint32_t mpu6050_read_accel(int16_t *ax,
                            int16_t *ay,
                            int16_t *az);

/*
 * Lit les valeurs brutes du gyroscope.
 */
uint32_t mpu6050_read_gyro(int16_t *gx,
                           int16_t *gy,
                           int16_t *gz);

#endif /* MPU6050_H */