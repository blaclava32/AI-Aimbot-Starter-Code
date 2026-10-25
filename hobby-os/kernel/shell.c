#include "shell.h"
#include "vga.h"
#include "keyboard.h"
#include "string.h"
#include "ports.h"

#define LINE_MAX 120
#define HISTORY_MAX 8

static char history[HISTORY_MAX][LINE_MAX];
static int history_count;
static uint32_t cmd_count;

static void print_banner(void) {
    vga_set_color(VGA_LIGHT_CYAN, VGA_BLACK);
    vga_writeln("  ____                  _     ___  ____");
    vga_writeln(" / ___| _ __   __ _ _ __| | __/ _ \\/ ___|");
    vga_writeln(" \\___ \\| '_ \\ / _` | '__| |/ / | | \\___ \\");
    vga_writeln("  ___) | |_) | (_| | |  |   <| |_| |___) |");
    vga_writeln(" |____/| .__/ \\__,_|_|  |_|\\_\\___/|____/");
    vga_set_color(VGA_YELLOW, VGA_BLACK);
    vga_writeln("       |_|  hobby kernel  -  x86 Multiboot2");
    vga_set_color(VGA_LIGHT_GREY, VGA_BLACK);
    vga_writeln("");
    vga_writeln("Type 'help' for commands. Welcome to your own OS.");
    vga_writeln("");
}

static void cmd_help(void) {
    vga_writeln("Commands:");
    vga_writeln("  help      - this list");
    vga_writeln("  clear     - clear the screen");
    vga_writeln("  echo ...  - print arguments");
    vga_writeln("  about     - OS info");
    vga_writeln("  color N   - text color 0-15");
    vga_writeln("  peek ADDR - read a byte (hex addr)");
    vga_writeln("  poke A V  - write byte V at ADDR (hex)");
    vga_writeln("  ticks     - rough busy-loop counter");
    vga_writeln("  history   - recent commands");
    vga_writeln("  reboot    - reset via keyboard controller");
    vga_writeln("  halt      - stop the CPU");
}

static void cmd_about(void) {
    vga_set_color(VGA_LIGHT_GREEN, VGA_BLACK);
    vga_writeln("SparkOS 0.1 - a tiny educational kernel");
    vga_set_color(VGA_LIGHT_GREY, VGA_BLACK);
    vga_writeln("Arch:     i386 (32-bit protected mode)");
    vga_writeln("Boot:     Multiboot2 via GRUB");
    vga_writeln("Console:  VGA text 80x25 @ 0xB8000");
    vga_writeln("Input:    PS/2 keyboard (polling)");
    vga_writeln("Built for fun - extend it!");
}

static uint32_t parse_hex(const char *s) {
    uint32_t v = 0;
    if (s[0] == '0' && (s[1] == 'x' || s[1] == 'X')) {
        s += 2;
    }
    while (*s) {
        char c = *s++;
        v <<= 4;
        if (c >= '0' && c <= '9') {
            v |= (uint32_t)(c - '0');
        } else if (c >= 'a' && c <= 'f') {
            v |= (uint32_t)(c - 'a' + 10);
        } else if (c >= 'A' && c <= 'F') {
            v |= (uint32_t)(c - 'A' + 10);
        } else {
            break;
        }
    }
    return v;
}

static const char *skip_spaces(const char *s) {
    while (*s == ' ') {
        s++;
    }
    return s;
}

static const char *next_token(const char *s, char *out, size_t out_sz) {
    size_t i = 0;
    s = skip_spaces(s);
    while (*s && *s != ' ' && i + 1 < out_sz) {
        out[i++] = *s++;
    }
    out[i] = '\0';
    return s;
}

static void push_history(const char *line) {
    if (!line[0]) {
        return;
    }
    int slot = history_count % HISTORY_MAX;
    strcpy(history[slot], line);
    history_count++;
}

static void cmd_history(void) {
    int start = history_count > HISTORY_MAX ? history_count - HISTORY_MAX : 0;
    for (int i = start; i < history_count; i++) {
        char num[16];
        itoa(i + 1, num, 10);
        vga_write("  ");
        vga_write(num);
        vga_write("  ");
        vga_writeln(history[i % HISTORY_MAX]);
    }
}

static void reboot(void) {
    vga_writeln("Rebooting...");
    /* Wait for keyboard controller input buffer clear, then pulse reset. */
    for (int i = 0; i < 100000; i++) {
        if ((inb(0x64) & 0x02) == 0) {
            break;
        }
    }
    outb(0x64, 0xFE);
    for (;;) {
        __asm__ volatile ("hlt");
    }
}

static void run_line(const char *line) {
    char cmd[32];
    const char *rest = next_token(line, cmd, sizeof(cmd));
    rest = skip_spaces(rest);

    if (!cmd[0]) {
        return;
    }
    cmd_count++;

    if (strcmp(cmd, "help") == 0) {
        cmd_help();
    } else if (strcmp(cmd, "clear") == 0) {
        vga_clear();
        print_banner();
    } else if (strcmp(cmd, "echo") == 0) {
        vga_writeln(rest);
    } else if (strcmp(cmd, "about") == 0) {
        cmd_about();
    } else if (strcmp(cmd, "color") == 0) {
        char arg[16];
        next_token(rest, arg, sizeof(arg));
        int n = 0;
        for (const char *p = arg; *p >= '0' && *p <= '9'; p++) {
            n = n * 10 + (*p - '0');
        }
        if (n < 0 || n > 15) {
            vga_writeln("color: use 0-15");
        } else {
            vga_set_color((uint8_t)n, VGA_BLACK);
            vga_writeln("Color set.");
        }
    } else if (strcmp(cmd, "peek") == 0) {
        char arg[32];
        next_token(rest, arg, sizeof(arg));
        uint32_t addr = parse_hex(arg);
        uint8_t value = *(volatile uint8_t *)(uintptr_t)addr;
        char buf[32];
        vga_write("0x");
        utoa(addr, buf, 16);
        vga_write(buf);
        vga_write(" = 0x");
        utoa(value, buf, 16);
        vga_writeln(buf);
    } else if (strcmp(cmd, "poke") == 0) {
        char a1[32], a2[32];
        rest = next_token(rest, a1, sizeof(a1));
        next_token(rest, a2, sizeof(a2));
        uint32_t addr = parse_hex(a1);
        uint8_t value = (uint8_t)parse_hex(a2);
        *(volatile uint8_t *)(uintptr_t)addr = value;
        vga_writeln("ok");
    } else if (strcmp(cmd, "ticks") == 0) {
        char buf[32];
        utoa(cmd_count, buf, 10);
        vga_write("commands run: ");
        vga_writeln(buf);
    } else if (strcmp(cmd, "history") == 0) {
        cmd_history();
    } else if (strcmp(cmd, "reboot") == 0) {
        reboot();
    } else if (strcmp(cmd, "halt") == 0) {
        vga_writeln("CPU halted. Reset the VM to continue.");
        for (;;) {
            __asm__ volatile ("cli; hlt");
        }
    } else {
        vga_set_color(VGA_LIGHT_RED, VGA_BLACK);
        vga_write("unknown command: ");
        vga_writeln(cmd);
        vga_set_color(VGA_LIGHT_GREY, VGA_BLACK);
        vga_writeln("Try 'help'.");
    }
}

static void read_line(char *buf, size_t max) {
    size_t len = 0;
    buf[0] = '\0';
    for (;;) {
        char c = keyboard_getchar();
        if (c == '\n') {
            vga_putc('\n');
            buf[len] = '\0';
            return;
        }
        if (c == '\b') {
            if (len > 0) {
                len--;
                buf[len] = '\0';
                vga_putc('\b');
            }
            continue;
        }
        if (c == 27) {
            /* ignore escape */
            continue;
        }
        if (c >= 32 && c < 127 && len + 1 < max) {
            buf[len++] = c;
            buf[len] = '\0';
            vga_putc(c);
        }
    }
}

void shell_run(void) {
    char line[LINE_MAX];
    print_banner();
    for (;;) {
        vga_set_color(VGA_LIGHT_GREEN, VGA_BLACK);
        vga_write("spark");
        vga_set_color(VGA_DARK_GREY, VGA_BLACK);
        vga_write(":");
        vga_set_color(VGA_LIGHT_CYAN, VGA_BLACK);
        vga_write("~");
        vga_set_color(VGA_LIGHT_GREY, VGA_BLACK);
        vga_write("$ ");
        read_line(line, sizeof(line));
        push_history(line);
        run_line(line);
    }
}
