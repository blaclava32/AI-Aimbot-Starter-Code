#include "pmm.h"
#include "string.h"

#define FRAME_SIZE 4096
#define MAX_FRAMES 8192 /* up to 32 MiB */

static uint8_t bitmap[MAX_FRAMES / 8];
static uint32_t nframes;
static uint32_t used;

static void set_frame(uint32_t i) {
    bitmap[i / 8] |= (uint8_t)(1u << (i % 8));
}

static void clear_frame(uint32_t i) {
    bitmap[i / 8] &= (uint8_t)~(1u << (i % 8));
}

static int test_frame(uint32_t i) {
    return bitmap[i / 8] & (1u << (i % 8));
}

void pmm_init(uint32_t mem_bytes) {
    nframes = mem_bytes / FRAME_SIZE;
    if (nframes > MAX_FRAMES) nframes = MAX_FRAMES;
    memset(bitmap, 0, sizeof(bitmap));
    used = 0;
    /* Reserve first 2 MiB (kernel + BIOS + heap start region) */
    uint32_t reserve = (2 * 1024 * 1024) / FRAME_SIZE;
    for (uint32_t i = 0; i < reserve && i < nframes; i++) {
        set_frame(i);
        used++;
    }
}

uint32_t pmm_alloc_frame(void) {
    for (uint32_t i = 0; i < nframes; i++) {
        if (!test_frame(i)) {
            set_frame(i);
            used++;
            return i * FRAME_SIZE;
        }
    }
    return 0;
}

void pmm_free_frame(uint32_t addr) {
    uint32_t i = addr / FRAME_SIZE;
    if (i >= nframes) return;
    if (test_frame(i)) {
        clear_frame(i);
        if (used) used--;
    }
}

void pmm_stats(uint32_t *total_frames, uint32_t *used_frames) {
    if (total_frames) *total_frames = nframes;
    if (used_frames) *used_frames = used;
}
