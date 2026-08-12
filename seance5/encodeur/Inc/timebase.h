#ifndef TIMEBASE_H
#define TIMEBASE_H

#include <stdint.h>

void timebase_init(void);
uint32_t timebase_get_us(void);

#endif