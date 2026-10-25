#ifndef SPARK_KEYBOARD_H
#define SPARK_KEYBOARD_H

#include "types.h"

void keyboard_init(void);
void keyboard_on_irq(void);
char keyboard_getchar(void);
int keyboard_try_getchar(char *out);
void keyboard_flush(void);

#endif
