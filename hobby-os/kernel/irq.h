#ifndef SPARK_IRQ_H
#define SPARK_IRQ_H

#include "types.h"

typedef struct registers {
    uint32_t gs, fs, es, ds;
    uint32_t edi, esi, ebp, esp_dummy, ebx, edx, ecx, eax;
    uint32_t int_no, err_code;
    uint32_t eip, cs, eflags, useresp, ss;
} registers_t;

typedef void (*irq_handler_t)(registers_t *regs);

void irq_init(void);
void irq_install_handler(uint8_t irq, irq_handler_t handler);
void isr_dispatch(registers_t *regs);

#endif
