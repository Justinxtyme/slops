#ifndef MEM_H
#define MEM_H

#include <stdint.h>
#include <stddef.h> 
#include "multiboot.h"



typedef struct Chunk {
    size_t size;
    int is_free;
    struct Chunk *next;
} Chunk;

void mem_init();

void add_region(uint64_t base, uint64_t length);

void init_allocator(const struct multiboot_tag_mmap* mmap); 

uint64_t alloc_frame();

void free_frame(uint64_t addr);

void* thralloc(size_t size);

size_t thralloc_total();

void* cralloc(size_t num, size_t size);

void tfree(void *ptr);

void* memset(void* bufptr, int value, size_t size);

#endif