#ifndef SPARK_PMM_H
#define SPARK_PMM_H

#include "types.h"

void pmm_init(uint32_t mem_bytes);
uint32_t pmm_alloc_frame(void);
void pmm_free_frame(uint32_t addr);
void pmm_stats(uint32_t *total_frames, uint32_t *used_frames);

#endif
