#include "pit.h"
#include "ports.h"

static volatile uint32_t ticks;
static uint32_t hz = 100;

void pit_init(uint32_t frequency) {
    hz = frequency ? frequency : 100;
    uint32_t divisor = 1193182 / hz;
    outb(0x43, 0x36);
    outb(0x40, (uint8_t)(divisor & 0xFF));
    outb(0x40, (uint8_t)((divisor >> 8) & 0xFF));
    ticks = 0;
}

void pit_on_tick(void) {
    ticks++;
}

uint32_t pit_ticks(void) {
    return ticks;
}

uint32_t pit_uptime_ms(void) {
    uint32_t t = ticks;
    return (t / hz) * 1000u + ((t % hz) * 1000u) / hz;
}
