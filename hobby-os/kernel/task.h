#ifndef SPARK_TASK_H
#define SPARK_TASK_H

#include "types.h"

#define TASK_MAX 16
#define TASK_STACK 8192

typedef enum {
    TASK_FREE = 0,
    TASK_READY,
    TASK_RUNNING,
    TASK_SLEEPING,
    TASK_ZOMBIE
} task_state_t;

typedef struct task {
    int id;
    char name[24];
    task_state_t state;
    uint32_t *stack;
    uint32_t esp;
    uint32_t sleep_until;
    uint32_t ticks_run;
    void (*entry)(void);
} task_t;

void task_init(void);
int task_create(const char *name, void (*entry)(void));
void task_yield(void);
void task_exit(void);
void task_on_timer(void);
void task_sleep_ms(uint32_t ms);
task_t *task_current(void);
void task_list(void);
int task_kill(int pid);
void task_start_scheduler(void);

#endif
