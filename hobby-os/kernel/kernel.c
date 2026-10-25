#include <stdint.h>
#include "vga.h"
#include "keyboard.h"
#include "shell.h"
#include "string.h"

#define MULTIBOOT2_BOOTLOADER_MAGIC 0x36d76289

void kernel_main(uint32_t magic, uint32_t multiboot_info) {
    (void)multiboot_info;

    vga_init();
    keyboard_init();

    if (magic != MULTIBOOT2_BOOTLOADER_MAGIC) {
        vga_set_color(VGA_LIGHT_RED, VGA_BLACK);
        vga_writeln("SparkOS: bad Multiboot2 magic — not booted by GRUB?");
        for (;;) {
            __asm__ volatile ("cli; hlt");
        }
    }

    shell_run();
}
