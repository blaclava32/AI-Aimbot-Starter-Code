#include "heap.h"
#include "string.h"
#include "ports.h"

typedef struct block {
    size_t size;
    int free;
    struct block *next;
} block_t;

static block_t *head;
static uint32_t heap_start, heap_end, heap_used;

void heap_init(uint32_t start, uint32_t size) {
    heap_start = start;
    heap_end = start + size;
    heap_used = 0;
    head = (block_t *)start;
    head->size = size - sizeof(block_t);
    head->free = 1;
    head->next = NULL;
}

static void split(block_t *b, size_t size) {
    if (b->size < size + sizeof(block_t) + 8) return;
    block_t *n = (block_t *)((uint8_t *)b + sizeof(block_t) + size);
    n->size = b->size - size - sizeof(block_t);
    n->free = 1;
    n->next = b->next;
    b->size = size;
    b->next = n;
}

void *kmalloc(size_t size) {
    if (size == 0) return NULL;
    size = (size + 7) & ~(size_t)7;
    cli();
    for (block_t *b = head; b; b = b->next) {
        if (b->free && b->size >= size) {
            split(b, size);
            b->free = 0;
            heap_used += b->size + sizeof(block_t);
            sti();
            return (void *)(b + 1);
        }
    }
    sti();
    return NULL;
}

void *kcalloc(size_t n, size_t size) {
    size_t total = n * size;
    void *p = kmalloc(total);
    if (p) memset(p, 0, total);
    return p;
}

void kfree(void *ptr) {
    if (!ptr) return;
    cli();
    block_t *b = ((block_t *)ptr) - 1;
    b->free = 1;
    if (heap_used >= b->size + sizeof(block_t))
        heap_used -= b->size + sizeof(block_t);
    else
        heap_used = 0;
    /* coalesce */
    for (block_t *c = head; c && c->next; ) {
        if (c->free && c->next->free) {
            c->size += sizeof(block_t) + c->next->size;
            c->next = c->next->next;
        } else {
            c = c->next;
        }
    }
    sti();
}

void heap_stats(uint32_t *used, uint32_t *total, uint32_t *free_bytes) {
    if (used) *used = heap_used;
    if (total) *total = heap_end - heap_start;
    if (free_bytes) *free_bytes = (heap_end - heap_start) - heap_used;
}
