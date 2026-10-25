#include "keyboard.h"
#include "ports.h"

#define KBD_DATA   0x60
#define KBD_STATUS 0x64
#define BUF_SIZE   256

static volatile char buf[BUF_SIZE];
static volatile uint32_t head, tail;
static int shift_held, ctrl_held, caps;

static const char map[] = {
    0,27,'1','2','3','4','5','6','7','8','9','0','-','=','\b',
    '\t','q','w','e','r','t','y','u','i','o','p','[',']','\n',
    0,'a','s','d','f','g','h','j','k','l',';','\'','`',
    0,'\\','z','x','c','v','b','n','m',',','.','/',0,'*',
    0,' '
};

static const char map_shift[] = {
    0,27,'!','@','#','$','%','^','&','*','(',')','_','+','\b',
    '\t','Q','W','E','R','T','Y','U','I','O','P','{','}','\n',
    0,'A','S','D','F','G','H','J','K','L',':','"','~',
    0,'|','Z','X','C','V','B','N','M','<','>','?',0,'*',
    0,' '
};

static void push_char(char c) {
    uint32_t next = (head + 1) % BUF_SIZE;
    if (next == tail) return; /* drop if full */
    buf[head] = c;
    head = next;
}

void keyboard_init(void) {
    head = tail = 0;
    shift_held = ctrl_held = caps = 0;
    while (inb(KBD_STATUS) & 1) (void)inb(KBD_DATA);
}

void keyboard_flush(void) {
    head = tail = 0;
}

void keyboard_on_irq(void) {
    uint8_t sc = inb(KBD_DATA);

    if (sc == 0x2A || sc == 0x36) { shift_held = 1; return; }
    if (sc == 0xAA || sc == 0xB6) { shift_held = 0; return; }
    if (sc == 0x1D) { ctrl_held = 1; return; }
    if (sc == 0x9D) { ctrl_held = 0; return; }
    if (sc == 0x3A) { caps = !caps; return; }
    if (sc & 0x80) return;
    if (sc >= sizeof(map)) return;

    char c = (shift_held ? map_shift[sc] : map[sc]);
    if (!c) return;
    if (caps && c >= 'a' && c <= 'z') c = (char)(c - 'a' + 'A');
    if (caps && c >= 'A' && c <= 'Z' && !shift_held) c = (char)(c - 'A' + 'a');
    if (ctrl_held && c >= 'a' && c <= 'z') c = (char)(c - 'a' + 1);
    if (ctrl_held && c >= 'A' && c <= 'Z') c = (char)(c - 'A' + 1);
    push_char(c);
}

int keyboard_try_getchar(char *out) {
    if (head == tail) return 0;
    *out = buf[tail];
    tail = (tail + 1) % BUF_SIZE;
    return 1;
}

char keyboard_getchar(void) {
    char c;
    for (;;) {
        if (keyboard_try_getchar(&c)) return c;
        hlt();
    }
}
