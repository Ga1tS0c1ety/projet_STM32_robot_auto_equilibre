#ifndef ENCODER_H
#define ENCODER_H

#include <stdint.h>

void encoder_init(void);
int32_t encoder_get_count(void);
int32_t encoder_compute_rpm(int32_t delta_count, uint32_t delta_us);

#endif