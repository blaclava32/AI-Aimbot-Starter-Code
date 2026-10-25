#ifndef SPARK_HEAP_H
#define SPARK_HEAP_H

#include "types.h"

void heap_init(uint32_t start, uint32_t size);
void *kmalloc(size_t size);
void *kcalloc(size_t n, size_t size);
void kfree(void *ptr);
void heap_stats(uint32_t *used, uint32_t *total, uint32_t *free_bytes);

#endif
