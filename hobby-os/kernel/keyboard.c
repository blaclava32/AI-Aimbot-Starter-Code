#include "keyboard.h"
#include "ports.h"

#define KBD_DATA 0x60
#define KBD_STATUS 0x64

static const char scancode_set1[] = {
    0,  27, '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '-', '=', '\b',
    '\t', 'q', 'w', 'e', 'r', 't', 'y', 'u', 'i', 'o', 'p', '[', ']', '\n',
    0, 'a', 's', 'd', 'f', 'g', 'h', 'j', 'k', 'l', ';', '\'', '`',
    0, '\\', 'z', 'x', 'c', 'v', 'b', 'n', 'm', ',', '.', '/', 0, '*',
    0, ' ',
};

static const char scancode_set1_shift[] = {
    0,  27, '!', '@', '#', '$', '%', '^', '&', '*', '(', ')', '_', '+', '\b',
    '\t', 'Q', 'W', 'E', 'R', 'T', 'Y', 'U', 'I', 'O', 'P', '{', '}', '\n',
    0, 'A', 'S', 'D', 'F', 'G', 'H', 'J', 'K', 'L', ':', '"', '~',
    0, '|', 'Z', 'X', 'C', 'V', 'B', 'N', 'M', '<', '>', '?', 0, '*',
    0, ' ',
};

static int shift_held;

void keyboard_init(void) {
    shift_held = 0;
    /* Drain any pending scancodes. */
    while (inb(KBD_STATUS) & 1) {
        (void)inb(KBD_DATA);
    }
}

static char translate(uint8_t scancode) {
    if (scancode == 0x2A || scancode == 0x36) {
        shift_held = 1;
        return 0;
    }
    if (scancode == 0xAA || scancode == 0xB6) {
        shift_held = 0;
        return 0;
    }
    /* Key release — ignore. */
    if (scancode & 0x80) {
        return 0;
    }
    if (scancode >= sizeof(scancode_set1)) {
        return 0;
    }
    return shift_held ? scancode_set1_shift[scancode] : scancode_set1[scancode];
}

int keyboard_try_getchar(char *out) {
    if (!(inb(KBD_STATUS) & 1)) {
        return 0;
    }
    uint8_t sc = inb(KBD_DATA);
    char c = translate(sc);
    if (!c) {
        return 0;
    }
    *out = c;
    return 1;
}

char keyboard_getchar(void) {
    char c;
    /* Busy-poll: IRQs are not enabled yet, so HLT would hang forever. */
    for (;;) {
        if (keyboard_try_getchar(&c)) {
            return c;
        }
        __asm__ volatile ("pause");
    }
}
