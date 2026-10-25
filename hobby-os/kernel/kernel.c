#include "types.h"
#include "string.h"
#include "vga.h"
#include "gdt.h"
#include "idt.h"
#include "pic.h"
#include "pit.h"
#include "irq.h"
#include "keyboard.h"
#include "heap.h"
#include "pmm.h"
#include "vfs.h"
#include "task.h"
#include "shell.h"
#include "ports.h"

#define MULTIBOOT2_MAGIC 0x36d76289

static void boot_log(const char *msg, bool ok) {
    vga_set_color(VGA_DARK_GREY, VGA_BLACK);
    vga_write(" [");
    if (ok) {
        vga_set_color(VGA_LIGHT_GREEN, VGA_BLACK);
        vga_write(" OK ");
    } else {
        vga_set_color(VGA_LIGHT_RED, VGA_BLACK);
        vga_write("FAIL");
    }
    vga_set_color(VGA_DARK_GREY, VGA_BLACK);
    vga_write("] ");
    vga_set_color(VGA_LIGHT_GREY, VGA_BLACK);
    vga_writeln(msg);
}

static void shell_task(void) {
    shell_run();
}

void kernel_main(uint32_t magic, uint32_t multiboot_info) {
    (void)multiboot_info;

    vga_init();
    vga_set_color(VGA_WHITE, VGA_BLUE);
    for (int i = 0; i < 80; i++) vga_putc(' ');
    vga_set_color(VGA_YELLOW, VGA_BLUE);
    vga_write(" SparkOS 1.0                                  booting...");
    vga_set_color(VGA_LIGHT_GREY, VGA_BLACK);
    vga_writeln("");
    vga_writeln("");

    if (magic != MULTIBOOT2_MAGIC) {
        boot_log("Multiboot2 magic", false);
        for (;;) { cli(); hlt(); }
    }
    boot_log("Multiboot2 handoff", true);

    gdt_init();
    boot_log("GDT flat segments", true);

    irq_init();
    idt_init();
    boot_log("IDT + exception gates", true);

    pic_init();
    boot_log("PIC remapped (IRQ0-15 -> INT 32-47)", true);

    pit_init(100);
    boot_log("PIT timer @ 100 Hz", true);

    keyboard_init();
    boot_log("PS/2 keyboard (IRQ1)", true);

    /* Heap starts at 2 MiB, 1 MiB size; PMM manages 32 MiB */
    pmm_init(32u * 1024u * 1024u);
    boot_log("Physical frame allocator (32 MiB)", true);

    heap_init(0x200000, 0x100000);
    boot_log("Kernel heap (1 MiB @ 2 MiB)", true);

    vfs_init();
    boot_log("Ramdisk VFS mounted at /", true);

    task_init();
    boot_log("Task scheduler ready", true);

    sti();
    boot_log("Interrupts enabled", true);

    vga_writeln("");
    vga_set_color(VGA_LIGHT_GREEN, VGA_BLACK);
    vga_writeln("SparkOS kernel initialization complete.");
    vga_set_color(VGA_LIGHT_GREY, VGA_BLACK);
    vga_writeln("Starting init (login + shell) and klogd...");
    vga_writeln("");

    /* Brief pause so boot messages are readable */
    uint32_t start = pit_ticks();
    while (pit_ticks() - start < 80) hlt();

    task_create("klogd", klogd_main);
    task_create("init", shell_task);
    task_start_scheduler();

    /* Should not return */
    for (;;) { hlt(); }
}
