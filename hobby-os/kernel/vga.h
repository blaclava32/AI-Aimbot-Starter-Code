#ifndef SPARK_VGA_H
#define SPARK_VGA_H

#include "types.h"

enum vga_color {
    VGA_BLACK = 0, VGA_BLUE, VGA_GREEN, VGA_CYAN,
    VGA_RED, VGA_MAGENTA, VGA_BROWN, VGA_LIGHT_GREY,
    VGA_DARK_GREY, VGA_LIGHT_BLUE, VGA_LIGHT_GREEN, VGA_LIGHT_CYAN,
    VGA_LIGHT_RED, VGA_LIGHT_MAGENTA, VGA_YELLOW, VGA_WHITE
};

void vga_init(void);
void vga_clear(void);
void vga_set_color(uint8_t fg, uint8_t bg);
uint8_t vga_get_fg(void);
void vga_putc(char c);
void vga_write(const char *s);
void vga_writeln(const char *s);
void vga_write_dec(uint32_t n);
void vga_write_hex(uint32_t n);
void vga_enable_cursor(bool enable);

#endif
