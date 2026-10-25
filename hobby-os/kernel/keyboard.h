#ifndef SPARK_KEYBOARD_H
#define SPARK_KEYBOARD_H

#include <stdint.h>

void keyboard_init(void);
/* Blocking read of one ASCII character (0 if non-printable / ignored). */
char keyboard_getchar(void);
/* Non-blocking: returns 1 and writes *out if a key is ready. */
int keyboard_try_getchar(char *out);

#endif
