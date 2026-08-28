#include "mpu6050.h"
#include "i2c.h"

/*
 * Adresse I2C du MPU6050.
 */
#define MPU6050_ADDR            0x68U

/*
 * Registres utilisés.
 */
#define MPU6050_PWR_MGMT_1      0x6BU

#define MPU6050_ACCEL_XOUT_H    0x3BU
#define MPU6050_GYRO_XOUT_H     0x43U

uint32_t mpu6050_init(void)
{
    /*
     * PWR_MGMT_1
     *
     * SLEEP = 0
     * → réveil du MPU6050.
     */
    return i2c1_write_register(MPU6050_ADDR,
                               MPU6050_PWR_MGMT_1,
                               0x00U);
}

uint32_t mpu6050_read_accel(int16_t *ax,
                            int16_t *ay,
                            int16_t *az)
{
    uint8_t donnees[6];
    uint32_t erreur;

    /*
     * Lire :
     *
     * 0x3B ACCEL_XOUT_H
     * 0x3C ACCEL_XOUT_L
     * 0x3D ACCEL_YOUT_H
     * 0x3E ACCEL_YOUT_L
     * 0x3F ACCEL_ZOUT_H
     * 0x40 ACCEL_ZOUT_L
     */
    erreur = i2c1_read_registers(MPU6050_ADDR,
                                 MPU6050_ACCEL_XOUT_H,
                                 donnees,
                                 6U);

    if (erreur != 0U)
    {
        return erreur;
    }

    /*
     * Reconstruction des trois valeurs
     * signées sur 16 bits.
     */
    *ax = (int16_t)(((uint16_t)donnees[0] << 8)
                   | donnees[1]);

    *ay = (int16_t)(((uint16_t)donnees[2] << 8)
                   | donnees[3]);

    *az = (int16_t)(((uint16_t)donnees[4] << 8)
                   | donnees[5]);

    return 0;
}

uint32_t mpu6050_read_gyro(int16_t *gx,
                           int16_t *gy,
                           int16_t *gz)
{
    uint8_t donnees[6];
    uint32_t erreur;

    /*
     * Lire :
     *
     * 0x43 GYRO_XOUT_H
     * 0x44 GYRO_XOUT_L
     * 0x45 GYRO_YOUT_H
     * 0x46 GYRO_YOUT_L
     * 0x47 GYRO_ZOUT_H
     * 0x48 GYRO_ZOUT_L
     */
    erreur = i2c1_read_registers(MPU6050_ADDR,
                                 MPU6050_GYRO_XOUT_H,
                                 donnees,
                                 6U);

    if (erreur != 0U)
    {
        return erreur;
    }

    *gx = (int16_t)(((uint16_t)donnees[0] << 8)
                   | donnees[1]);

    *gy = (int16_t)(((uint16_t)donnees[2] << 8)
                   | donnees[3]);

    *gz = (int16_t)(((uint16_t)donnees[4] << 8)
                   | donnees[5]);

    return 0;
}