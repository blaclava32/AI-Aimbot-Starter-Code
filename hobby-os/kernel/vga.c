#include "vga.h"
#include "ports.h"
#include "string.h"

#define VGA_WIDTH  80
#define VGA_HEIGHT 25
#define VGA_MEMORY ((volatile uint16_t *)0xB8000)

static size_t row, col;
static uint8_t color;
static volatile uint16_t *buffer;
static bool cursor_on = true;

static uint8_t make_color(uint8_t fg, uint8_t bg) {
    return (uint8_t)(fg | (bg << 4));
}

static uint16_t make_entry(char c, uint8_t color_byte) {
    return (uint16_t)c | ((uint16_t)color_byte << 8);
}

void vga_set_color(uint8_t fg, uint8_t bg) {
    color = make_color(fg, bg);
}

uint8_t vga_get_fg(void) {
    return color & 0x0F;
}

void vga_move_cursor(void) {
    if (!cursor_on) return;
    uint16_t pos = (uint16_t)(row * VGA_WIDTH + col);
    outb(0x3D4, 0x0F);
    outb(0x3D5, (uint8_t)(pos & 0xFF));
    outb(0x3D4, 0x0E);
    outb(0x3D5, (uint8_t)((pos >> 8) & 0xFF));
}

void vga_enable_cursor(bool enable) {
    cursor_on = enable;
    if (!enable) {
        outb(0x3D4, 0x0A);
        outb(0x3D5, 0x20);
    } else {
        outb(0x3D4, 0x0A);
        outb(0x3D5, 0x0E);
        outb(0x3D4, 0x0B);
        outb(0x3D5, 0x0F);
        vga_move_cursor();
    }
}

void vga_clear(void) {
    for (size_t y = 0; y < VGA_HEIGHT; y++)
        for (size_t x = 0; x < VGA_WIDTH; x++)
            buffer[y * VGA_WIDTH + x] = make_entry(' ', color);
    row = col = 0;
    vga_move_cursor();
}

void vga_init(void) {
    buffer = VGA_MEMORY;
    row = col = 0;
    vga_set_color(VGA_LIGHT_GREY, VGA_BLACK);
    vga_enable_cursor(true);
    vga_clear();
}

static void scroll(void) {
    for (size_t y = 1; y < VGA_HEIGHT; y++)
        for (size_t x = 0; x < VGA_WIDTH; x++)
            buffer[(y - 1) * VGA_WIDTH + x] = buffer[y * VGA_WIDTH + x];
    for (size_t x = 0; x < VGA_WIDTH; x++)
        buffer[(VGA_HEIGHT - 1) * VGA_WIDTH + x] = make_entry(' ', color);
    row = VGA_HEIGHT - 1;
}

void vga_putc(char c) {
    if (c == '\n') {
        col = 0;
        if (++row == VGA_HEIGHT) scroll();
    } else if (c == '\r') {
        col = 0;
    } else if (c == '\b') {
        if (col > 0) {
            col--;
            buffer[row * VGA_WIDTH + col] = make_entry(' ', color);
        }
    } else if (c == '\t') {
        size_t next = (col + 4) & ~(size_t)3;
        while (col < next) vga_putc(' ');
    } else {
        buffer[row * VGA_WIDTH + col] = make_entry(c, color);
        if (++col == VGA_WIDTH) {
            col = 0;
            if (++row == VGA_HEIGHT) scroll();
        }
    }
    vga_move_cursor();
}

void vga_write(const char *s) {
    while (*s) vga_putc(*s++);
}

void vga_writeln(const char *s) {
    vga_write(s);
    vga_putc('\n');
}

void vga_write_dec(uint32_t n) {
    char buf[16];
    utoa(n, buf, 10);
    vga_write(buf);
}

void vga_write_hex(uint32_t n) {
    char buf[16];
    vga_write("0x");
    utoa(n, buf, 16);
    vga_write(buf);
}
