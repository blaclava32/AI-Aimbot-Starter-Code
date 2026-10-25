#include "shell.h"
#include "vga.h"
#include "keyboard.h"
#include "string.h"
#include "ports.h"
#include "vfs.h"
#include "heap.h"
#include "pmm.h"
#include "pit.h"
#include "task.h"

#define LINE_MAX 160
#define HIST_MAX 16

static char cwd[VFS_PATH_MAX] = "/home/spark";
static char history[HIST_MAX][LINE_MAX];
static int hist_count;

static void banner(void) {
    vga_set_color(VGA_LIGHT_CYAN, VGA_BLACK);
    vga_writeln("  ____                  _     ___  ____");
    vga_writeln(" / ___| _ __   __ _ _ __| | __/ _ \\/ ___|");
    vga_writeln(" \\___ \\| '_ \\ / _` | '__| |/ / | | \\___ \\");
    vga_writeln("  ___) | |_) | (_| | |  |   <| |_| |___) |");
    vga_writeln(" |____/| .__/ \\__,_|_|  |_|\\_\\___/|____/");
    vga_set_color(VGA_YELLOW, VGA_BLACK);
    vga_writeln("       |_|  SparkOS 1.0  -  complete hobby OS");
    vga_set_color(VGA_LIGHT_GREY, VGA_BLACK);
    vga_writeln("");
}

static const char *skip(const char *s) {
    while (*s == ' ') s++;
    return s;
}

static const char *token(const char *s, char *out, size_t n) {
    size_t i = 0;
    s = skip(s);
    while (*s && *s != ' ' && i + 1 < n) out[i++] = *s++;
    out[i] = 0;
    return s;
}

static void push_hist(const char *line) {
    if (!line[0]) return;
    strcpy(history[hist_count % HIST_MAX], line);
    hist_count++;
}

static void print_uptime(void) {
    uint32_t ms = pit_uptime_ms();
    uint32_t s = ms / 1000;
    uint32_t m = s / 60;
    uint32_t h = m / 60;
    vga_write("up ");
    vga_write_dec(h); vga_write("h ");
    vga_write_dec(m % 60); vga_write("m ");
    vga_write_dec(s % 60); vga_write("s (");
    vga_write_dec((uint32_t)pit_ticks());
    vga_writeln(" ticks)");
}

static void cmd_help(void) {
    vga_writeln("SparkOS command reference");
    vga_writeln("  help                 show this help");
    vga_writeln("  clear                clear screen");
    vga_writeln("  echo [text]          print text");
    vga_writeln("  uname                system identity");
    vga_writeln("  uptime / date        timer uptime");
    vga_writeln("  free                 memory stats");
    vga_writeln("  ps                   list processes");
    vga_writeln("  kill <pid>           kill a process");
    vga_writeln("  sleep <ms>           sleep current task");
    vga_writeln("  ls [path]            list directory");
    vga_writeln("  cd [path]            change directory");
    vga_writeln("  pwd                  print working directory");
    vga_writeln("  cat <file>           show file");
    vga_writeln("  write <file> <text>  overwrite file");
    vga_writeln("  append <file> <text> append to file");
    vga_writeln("  touch <file>         create empty file");
    vga_writeln("  mkdir <dir>          create directory");
    vga_writeln("  rm <path>            remove file/empty dir");
    vga_writeln("  edit <file>          simple line editor");
    vga_writeln("  tree [path]          show filesystem tree");
    vga_writeln("  history              command history");
    vga_writeln("  whoami / hostname    identity");
    vga_writeln("  calc <a> <+|-|*|/> <b>");
    vga_writeln("  poke / peek          raw memory access");
    vga_writeln("  reboot / halt        power controls");
    vga_writeln("  spawn <name>         start demo background task");
}

static void cmd_uname(void) {
    vga_writeln("SparkOS 1.0.0 i386 sparkos GNU/Spark");
    vga_writeln("Kernel: SparkOS kernel 1.0 (preemptive timer, VFS, heap, tasks)");
}

static void cmd_free(void) {
    uint32_t used, total, freeb, tf, uf;
    heap_stats(&used, &total, &freeb);
    pmm_stats(&tf, &uf);
    vga_write("Heap:  used="); vga_write_dec(used);
    vga_write("  free="); vga_write_dec(freeb);
    vga_write("  total="); vga_write_dec(total); vga_writeln(" B");
    vga_write("Frames:"); vga_write_dec(uf); vga_write("/");
    vga_write_dec(tf); vga_write(" used (");
    vga_write_dec(tf ? (uf * 100) / tf : 0); vga_writeln("%)");
}

static void cat_node(vfs_node_t *n) {
    if (!n) { vga_writeln("cat: not found"); return; }
    if (vfs_is_dir(n)) { vga_writeln("cat: is a directory"); return; }
    if (!n->data || n->size == 0) return;
    for (size_t i = 0; i < n->size; i++) vga_putc((char)n->data[i]);
    if (n->size == 0 || n->data[n->size - 1] != '\n') vga_putc('\n');
}

static void tree_rec(vfs_node_t *n, int depth) {
    if (!n) return;
    for (int i = 0; i < depth; i++) vga_write("  ");
    vga_set_color(n->type == VFS_DIR ? VGA_LIGHT_BLUE : VGA_LIGHT_GREY, VGA_BLACK);
    vga_writeln(n->name[0] ? n->name : "/");
    vga_set_color(VGA_LIGHT_GREY, VGA_BLACK);
    if (n->type == VFS_DIR)
        for (vfs_node_t *c = n->child; c; c = c->sibling)
            tree_rec(c, depth + 1);
}

static void edit_file(const char *path) {
    vfs_node_t *n = vfs_resolve(cwd, path);
    if (!n) n = vfs_create(cwd, path);
    if (!n || vfs_is_dir(n)) { vga_writeln("edit: cannot open"); return; }

    vga_writeln("-- editor: type lines, empty line or . alone to save --");
    if (n->size) {
        vga_set_color(VGA_DARK_GREY, VGA_BLACK);
        cat_node(n);
        vga_set_color(VGA_LIGHT_GREY, VGA_BLACK);
    }

    char body[VFS_FILE_MAX];
    size_t len = 0;
    body[0] = 0;
    for (;;) {
        vga_write("| ");
        char line[LINE_MAX];
        size_t li = 0;
        line[0] = 0;
        for (;;) {
            char c = keyboard_getchar();
            if (c == '\n') { vga_putc('\n'); break; }
            if (c == '\b') {
                if (li) { li--; line[li] = 0; vga_putc('\b'); }
                continue;
            }
            if (c >= 32 && c < 127 && li + 1 < sizeof(line)) {
                line[li++] = c; line[li] = 0; vga_putc(c);
            }
        }
        if (line[0] == 0 || (line[0] == '.' && line[1] == 0)) break;
        size_t add = strlen(line);
        if (len + add + 2 >= sizeof(body)) { vga_writeln("edit: file too large"); break; }
        memcpy(body + len, line, add);
        len += add;
        body[len++] = '\n';
        body[len] = 0;
    }
    vfs_write(n, body, len, false);
    vga_writeln("saved.");
}

static void demo_worker(void) {
    for (int i = 0; i < 20; i++) {
        task_sleep_ms(200);
    }
    task_exit();
}

static void reboot(void) {
    vga_writeln("Rebooting...");
    for (int i = 0; i < 100000; i++)
        if ((inb(0x64) & 2) == 0) break;
    outb(0x64, 0xFE);
    for (;;) { cli(); hlt(); }
}

static void run_line(const char *line) {
    char cmd[32];
    const char *rest = token(line, cmd, sizeof(cmd));
    rest = skip(rest);
    if (!cmd[0]) return;

    if (strcmp(cmd, "help") == 0) cmd_help();
    else if (strcmp(cmd, "clear") == 0) { vga_clear(); banner(); }
    else if (strcmp(cmd, "echo") == 0) vga_writeln(rest);
    else if (strcmp(cmd, "uname") == 0) cmd_uname();
    else if (strcmp(cmd, "uptime") == 0 || strcmp(cmd, "date") == 0) print_uptime();
    else if (strcmp(cmd, "free") == 0) cmd_free();
    else if (strcmp(cmd, "ps") == 0) task_list();
    else if (strcmp(cmd, "pwd") == 0) vga_writeln(cwd);
    else if (strcmp(cmd, "whoami") == 0) vga_writeln("spark");
    else if (strcmp(cmd, "hostname") == 0) {
        vfs_node_t *n = vfs_resolve("/", "/etc/hostname");
        if (n) cat_node(n); else vga_writeln("sparkos");
    }
    else if (strcmp(cmd, "ls") == 0) {
        char arg[VFS_PATH_MAX];
        token(rest, arg, sizeof(arg));
        vfs_list(vfs_resolve(cwd, arg[0] ? arg : "."));
    }
    else if (strcmp(cmd, "cd") == 0) {
        char arg[VFS_PATH_MAX];
        token(rest, arg, sizeof(arg));
        if (!arg[0]) { strcpy(cwd, "/home/spark"); return; }
        vfs_node_t *n = vfs_resolve(cwd, arg);
        if (!n || !vfs_is_dir(n)) { vga_writeln("cd: no such directory"); return; }
        vfs_normalize(cwd, arg, cwd, sizeof(cwd));
    }
    else if (strcmp(cmd, "cat") == 0) {
        char arg[VFS_PATH_MAX];
        token(rest, arg, sizeof(arg));
        cat_node(vfs_resolve(cwd, arg));
    }
    else if (strcmp(cmd, "touch") == 0) {
        char arg[VFS_PATH_MAX];
        token(rest, arg, sizeof(arg));
        if (!vfs_create(cwd, arg)) vga_writeln("touch: failed");
    }
    else if (strcmp(cmd, "mkdir") == 0) {
        char arg[VFS_PATH_MAX];
        token(rest, arg, sizeof(arg));
        if (!vfs_mkdir(cwd, arg)) vga_writeln("mkdir: failed");
    }
    else if (strcmp(cmd, "rm") == 0) {
        char arg[VFS_PATH_MAX];
        token(rest, arg, sizeof(arg));
        if (vfs_remove(cwd, arg) < 0) vga_writeln("rm: failed");
    }
    else if (strcmp(cmd, "write") == 0 || strcmp(cmd, "append") == 0) {
        char arg[VFS_PATH_MAX];
        rest = token(rest, arg, sizeof(arg));
        rest = skip(rest);
        vfs_node_t *n = vfs_create(cwd, arg);
        if (!n) { vga_writeln("write: failed"); return; }
        vfs_write(n, rest, strlen(rest), strcmp(cmd, "append") == 0);
        /* add trailing newline */
        vfs_write(n, "\n", 1, true);
    }
    else if (strcmp(cmd, "edit") == 0) {
        char arg[VFS_PATH_MAX];
        token(rest, arg, sizeof(arg));
        edit_file(arg);
    }
    else if (strcmp(cmd, "tree") == 0) {
        char arg[VFS_PATH_MAX];
        token(rest, arg, sizeof(arg));
        vfs_node_t *n = vfs_resolve(cwd, arg[0] ? arg : "/");
        tree_rec(n, 0);
    }
    else if (strcmp(cmd, "history") == 0) {
        int start = hist_count > HIST_MAX ? hist_count - HIST_MAX : 0;
        for (int i = start; i < hist_count; i++) {
            vga_write_dec((uint32_t)(i + 1));
            vga_write("  ");
            vga_writeln(history[i % HIST_MAX]);
        }
    }
    else if (strcmp(cmd, "kill") == 0) {
        char arg[16];
        token(rest, arg, sizeof(arg));
        if (task_kill(atoi(arg)) < 0) vga_writeln("kill: no such pid");
    }
    else if (strcmp(cmd, "sleep") == 0) {
        char arg[16];
        token(rest, arg, sizeof(arg));
        task_sleep_ms((uint32_t)atoi(arg));
    }
    else if (strcmp(cmd, "spawn") == 0) {
        char arg[24];
        token(rest, arg, sizeof(arg));
        if (!arg[0]) strcpy(arg, "worker");
        int pid = task_create(arg, demo_worker);
        if (pid < 0) vga_writeln("spawn: failed");
        else { vga_write("spawned pid "); vga_write_dec((uint32_t)pid); vga_putc('\n'); }
    }
    else if (strcmp(cmd, "calc") == 0) {
        char a[16], op[8], b[16];
        rest = token(rest, a, sizeof(a));
        rest = token(rest, op, sizeof(op));
        token(rest, b, sizeof(b));
        int x = atoi(a), y = atoi(b), r = 0;
        if (op[0] == '+') r = x + y;
        else if (op[0] == '-') r = x - y;
        else if (op[0] == '*') r = x * y;
        else if (op[0] == '/' && y) r = x / y;
        else { vga_writeln("calc: usage calc 3 + 4"); return; }
        char buf[16]; itoa(r, buf, 10); vga_writeln(buf);
    }
    else if (strcmp(cmd, "peek") == 0) {
        char arg[32]; token(rest, arg, sizeof(arg));
        uint32_t addr = parse_hex(arg);
        vga_write_hex(addr); vga_write(" = ");
        vga_write_hex(*(volatile uint8_t *)(uintptr_t)addr); vga_putc('\n');
    }
    else if (strcmp(cmd, "poke") == 0) {
        char a1[32], a2[32];
        rest = token(rest, a1, sizeof(a1));
        token(rest, a2, sizeof(a2));
        *(volatile uint8_t *)(uintptr_t)parse_hex(a1) = (uint8_t)parse_hex(a2);
        vga_writeln("ok");
    }
    else if (strcmp(cmd, "reboot") == 0) reboot();
    else if (strcmp(cmd, "halt") == 0) {
        vga_writeln("System halted.");
        for (;;) { cli(); hlt(); }
    }
    else {
        vga_set_color(VGA_LIGHT_RED, VGA_BLACK);
        vga_write("sparksh: "); vga_write(cmd); vga_writeln(": command not found");
        vga_set_color(VGA_LIGHT_GREY, VGA_BLACK);
    }
}

static void read_line(char *buf, size_t max) {
    size_t len = 0;
    buf[0] = 0;
    for (;;) {
        char c = keyboard_getchar();
        if (c == '\n') { vga_putc('\n'); buf[len] = 0; return; }
        if (c == '\b') {
            if (len) { len--; buf[len] = 0; vga_putc('\b'); }
            continue;
        }
        if (c == 3) { /* Ctrl-C */
            vga_writeln("^C");
            buf[0] = 0;
            return;
        }
        if (c >= 32 && c < 127 && len + 1 < max) {
            buf[len++] = c; buf[len] = 0; vga_putc(c);
        }
    }
}

void klogd_main(void) {
    vfs_node_t *log = vfs_resolve("/", "/var/log");
    for (;;) {
        task_sleep_ms(5000);
        if (!log) log = vfs_resolve("/", "/var/log");
        if (log) {
            char line[64];
            char num[16];
            strcpy(line, "klogd: heartbeat tick=");
            utoa((uint32_t)pit_ticks(), num, 10);
            strcat(line, num);
            strcat(line, "\n");
            vfs_write(log, line, strlen(line), true);
        }
    }
}

static void login(void) {
    vga_clear();
    banner();
    vfs_node_t *motd = vfs_resolve("/", "/etc/motd");
    if (motd) {
        vga_set_color(VGA_LIGHT_GREEN, VGA_BLACK);
        cat_node(motd);
        vga_set_color(VGA_LIGHT_GREY, VGA_BLACK);
    }
    vga_writeln("sparkos login: spark (press Enter)");
    char discard[64];
    read_line(discard, sizeof(discard));
    vga_writeln("Password: (none)");
    vga_writeln("");
    vga_set_color(VGA_LIGHT_GREEN, VGA_BLACK);
    vga_writeln("Last login: just now on tty1");
    vga_set_color(VGA_LIGHT_GREY, VGA_BLACK);
    vga_writeln("Home directory: /home/spark");
    vga_writeln("");
}

void shell_run(void) {
    login();
    banner();
    char line[LINE_MAX];
    for (;;) {
        vga_set_color(VGA_LIGHT_GREEN, VGA_BLACK);
        vga_write("spark");
        vga_set_color(VGA_DARK_GREY, VGA_BLACK);
        vga_write("@");
        vga_set_color(VGA_LIGHT_CYAN, VGA_BLACK);
        vga_write("sparkos");
        vga_set_color(VGA_LIGHT_GREY, VGA_BLACK);
        vga_write(":");
        vga_set_color(VGA_LIGHT_BLUE, VGA_BLACK);
        vga_write(cwd);
        vga_set_color(VGA_LIGHT_GREY, VGA_BLACK);
        vga_write("$ ");
        read_line(line, sizeof(line));
        push_hist(line);
        run_line(line);
        task_yield();
    }
}
