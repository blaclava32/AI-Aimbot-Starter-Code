#ifndef SPARK_PIC_H
#define SPARK_PIC_H

#include "types.h"

void pic_init(void);
void pic_eoi(uint8_t irq);
void pic_set_mask(uint16_t mask);

#endif
