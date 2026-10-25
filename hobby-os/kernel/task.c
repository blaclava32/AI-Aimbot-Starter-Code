#include "task.h"
#include "heap.h"
#include "pit.h"
#include "vga.h"
#include "string.h"
#include "ports.h"

static task_t tasks[TASK_MAX];
static int current = -1;
static int next_id = 1;
static volatile int preempt_counter;

extern void task_switch_asm(uint32_t *old_esp, uint32_t new_esp);

void task_init(void) {
    memset(tasks, 0, sizeof(tasks));
    current = -1;
    next_id = 1;
    preempt_counter = 0;
}

task_t *task_current(void) {
    if (current < 0) return NULL;
    return &tasks[current];
}

static int alloc_slot(void) {
    for (int i = 0; i < TASK_MAX; i++)
        if (tasks[i].state == TASK_FREE) return i;
    return -1;
}

static void task_trampoline(void) {
    task_t *t = task_current();
    if (t && t->entry) t->entry();
    task_exit();
}

int task_create(const char *name, void (*entry)(void)) {
    int slot = alloc_slot();
    if (slot < 0) return -1;
    task_t *t = &tasks[slot];
    memset(t, 0, sizeof(*t));
    t->id = next_id++;
    strncpy(t->name, name, sizeof(t->name) - 1);
    t->entry = entry;
    t->stack = kmalloc(TASK_STACK);
    if (!t->stack) { t->state = TASK_FREE; return -1; }
    memset(t->stack, 0, TASK_STACK);

    /* Build initial stack: fake saved regs for task_switch_asm */
    uint32_t *sp = (uint32_t *)((uint8_t *)t->stack + TASK_STACK);
    *--sp = (uint32_t)task_trampoline; /* return EIP */
    *--sp = 0; /* EBX */
    *--sp = 0; /* ESI */
    *--sp = 0; /* EDI */
    *--sp = 0; /* EBP */
    t->esp = (uint32_t)sp;
    t->state = TASK_READY;
    return t->id;
}

static int pick_next(void) {
    if (current < 0) {
        for (int i = 0; i < TASK_MAX; i++)
            if (tasks[i].state == TASK_READY) return i;
        return -1;
    }
    for (int i = 1; i <= TASK_MAX; i++) {
        int idx = (current + i) % TASK_MAX;
        if (tasks[idx].state == TASK_READY ||
            (tasks[idx].state == TASK_SLEEPING &&
             pit_uptime_ms() >= tasks[idx].sleep_until)) {
            if (tasks[idx].state == TASK_SLEEPING)
                tasks[idx].state = TASK_READY;
            return idx;
        }
    }
    return current;
}

void task_yield(void) {
    int next = pick_next();
    if (next < 0 || next == current) return;
    int prev = current;
    if (prev >= 0 && tasks[prev].state == TASK_RUNNING)
        tasks[prev].state = TASK_READY;
    tasks[next].state = TASK_RUNNING;
    current = next;
    if (prev < 0) {
        uint32_t dummy;
        task_switch_asm(&dummy, tasks[next].esp);
    } else {
        task_switch_asm(&tasks[prev].esp, tasks[next].esp);
    }
}

void task_exit(void) {
    if (current < 0) return;
    tasks[current].state = TASK_ZOMBIE;
    /* free stack later via reaper; keep for simplicity until reused */
    int next = pick_next();
    if (next < 0 || next == current) {
        current = -1;
        for (;;) { sti(); hlt(); }
    }
    int prev = current;
    tasks[next].state = TASK_RUNNING;
    current = next;
    uint32_t junk;
    task_switch_asm(&junk, tasks[next].esp);
    (void)prev;
}

void task_sleep_ms(uint32_t ms) {
    if (current < 0) return;
    tasks[current].sleep_until = pit_uptime_ms() + ms;
    tasks[current].state = TASK_SLEEPING;
    task_yield();
}

void task_on_timer(void) {
    if (current >= 0) tasks[current].ticks_run++;
    preempt_counter++;
    if (preempt_counter >= 5) { /* ~50ms at 100Hz */
        preempt_counter = 0;
        /* Cooperative hint only — full preempt needs careful IRQ reentry.
           We just wake sleepers here; shell yields explicitly. */
        for (int i = 0; i < TASK_MAX; i++) {
            if (tasks[i].state == TASK_SLEEPING &&
                pit_uptime_ms() >= tasks[i].sleep_until)
                tasks[i].state = TASK_READY;
        }
    }
}

void task_list(void) {
    vga_writeln("PID  STATE     TICKS  NAME");
    for (int i = 0; i < TASK_MAX; i++) {
        if (tasks[i].state == TASK_FREE) continue;
        const char *st = "?";
        switch (tasks[i].state) {
            case TASK_READY: st = "ready"; break;
            case TASK_RUNNING: st = "run"; break;
            case TASK_SLEEPING: st = "sleep"; break;
            case TASK_ZOMBIE: st = "zombie"; break;
            default: break;
        }
        vga_write_dec((uint32_t)tasks[i].id);
        vga_write("   ");
        vga_write(st);
        vga_write("      ");
        vga_write_dec(tasks[i].ticks_run);
        vga_write("   ");
        vga_writeln(tasks[i].name);
    }
}

int task_kill(int pid) {
    for (int i = 0; i < TASK_MAX; i++) {
        if (tasks[i].state != TASK_FREE && tasks[i].id == pid) {
            if (i == current) { task_exit(); return 0; }
            tasks[i].state = TASK_ZOMBIE;
            return 0;
        }
    }
    return -1;
}

void task_start_scheduler(void) {
    task_yield();
}
