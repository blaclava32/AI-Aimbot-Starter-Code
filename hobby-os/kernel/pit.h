#ifndef SPARK_PIT_H
#define SPARK_PIT_H

#include "types.h"

void pit_init(uint32_t hz);
uint32_t pit_ticks(void);
uint32_t pit_uptime_ms(void);
void pit_on_tick(void);

#endif
