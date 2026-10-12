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

float mpu6050_accel_to_g(int16_t valeur_brute);

float mpu6050_gyro_to_dps(int16_t valeur_brute);

uint32_t mpu6050_calibrate_gyro(int16_t *offset_gx,
                                int16_t *offset_gy,
                                int16_t *offset_gz);



#endif /* MPU6050_H */