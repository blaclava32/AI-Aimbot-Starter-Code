#include "irq.h"
#include "pic.h"
#include "pit.h"
#include "keyboard.h"
#include "vga.h"
#include "task.h"
#include "string.h"
#include "ports.h"

static irq_handler_t handlers[16];

void irq_install_handler(uint8_t irq, irq_handler_t handler) {
    if (irq < 16) handlers[irq] = handler;
}

static void exception_panic(registers_t *regs) {
    static const char *names[] = {
        "Divide by zero", "Debug", "NMI", "Breakpoint", "Overflow",
        "Bound range", "Invalid opcode", "Device not available",
        "Double fault", "Coprocessor overrun", "Invalid TSS",
        "Segment not present", "Stack fault", "General protection",
        "Page fault", "Reserved", "x87 FP", "Alignment check",
        "Machine check", "SIMD FP"
    };
    vga_set_color(VGA_LIGHT_RED, VGA_BLACK);
    vga_writeln("");
    vga_write("KERNEL PANIC: ");
    if (regs->int_no < 20) vga_writeln(names[regs->int_no]);
    else vga_writeln("Unknown exception");
    char buf[32];
    vga_write(" int="); utoa(regs->int_no, buf, 10); vga_write(buf);
    vga_write(" err=0x"); utoa(regs->err_code, buf, 16); vga_write(buf);
    vga_write(" eip=0x"); utoa(regs->eip, buf, 16); vga_writeln(buf);
    for (;;) {
        cli();
        hlt();
    }
}

void isr_dispatch(registers_t *regs) {
    if (regs->int_no < 32) {
        exception_panic(regs);
        return;
    }

    uint8_t irq = (uint8_t)(regs->int_no - 32);
    if (irq < 16 && handlers[irq]) {
        handlers[irq](regs);
    }

    /* Default built-in handlers if none installed */
    if (irq == 0) {
        pit_on_tick();
        task_on_timer();
    } else if (irq == 1) {
        keyboard_on_irq();
    }

    pic_eoi(irq);
}

void irq_init(void) {
    for (int i = 0; i < 16; i++) handlers[i] = 0;
}
