#include "mem.h"
#include "serial.h"
#include "multiboot.h"


#define MAX_PAGES 1048576 // for 4 GiB of RAM
#define HEAP_SIZE 4096
uint8_t page_bitmap[MAX_PAGES / 8]; // 128 KiB


extern uint8_t* _heap_start;

Chunk *free_head;

void mem_init(){
    free_head = (Chunk*)_heap_start;
    free_head->size = HEAP_SIZE - sizeof(Chunk);
    free_head->is_free = 1;
    free_head->next = NULL;
}

void* thralloc(size_t size) {
    Chunk *current = free_head;
    Chunk *prev = NULL;
    while (current) {
        if ((current->is_free) && (current->size >= size)) {
            if (current->size >= size + sizeof(Chunk)) {
                Chunk *new_chunk = (Chunk *)((uint8_t *)(current + 1) + size);
                new_chunk->size = current->size - size - sizeof(Chunk);
                new_chunk->is_free = 1;
                new_chunk->next = current->next;

                current->size = size;
                current->next = new_chunk;
            }
            current->is_free = 0;
            return (void*)(current + 1);
        }
        current = current->next;
    }
    return (void*)0; // no suitable chunk found
}

size_t thralloc_total() {
    size_t total = 0;
    Chunk* current = free_head;

    while (current) {
        if (!current->is_free) {
            total += current->size + sizeof(Chunk);
        }
        current = current->next;
    }

    sfprint("Total memory allocated: %8\n", total);
    return total;
}


int cralloc(size_t size) {
    Chunk *current = free_head;
    Chunk *prev = NULL;
    while (current) {
        if ((current->is_free) && (current->size >= size)) {
        }
    }
}

void add_region(uint64_t base, uint64_t length) {
    uint64_t start_page = base / 4096;
    uint64_t end_page = (base + length) / 4096;

    for (uint64_t i = start_page; i < end_page; i++) {
        page_bitmap[i / 8] &= ~(1 << (i % 8)); // mark as free
    }
}

uint64_t alloc_frame() {
    for (uint64_t i = 0; i < MAX_PAGES; i++) {
        if ((page_bitmap[i / 8] & (1 << (i % 8))) == 0) {
            page_bitmap[i / 8] |= (1 << (i % 8)); // mark as used
            return i * 4096;
        }
    }
    return 0; // out of memory
}

void free_frame(uint64_t addr) {
    uint64_t i = addr / 4096;
    page_bitmap[i / 8] &= ~(1 << (i % 8));
}


void init_allocator(const struct multiboot_tag_mmap* mmap) {
    size_t count = (mmap->size - sizeof(*mmap)) / mmap->entry_size;

    for (size_t i = 0; i < count; i++) {
        const struct multiboot_mmap_entry* entry = (const void*)mmap->entries + i * mmap->entry_size;
        
        if (entry->type == 1) {
            add_region(entry->base_addr, entry->length);
            sfprint("type: %8\naddress: %8\nlength: %8\nreserved: %8\n\n", entry->type, entry->base_addr, entry->length, entry->reserved);
        } else {
            sfprint("type: %8\naddress: %8\nlength: %8\nreserved: %8\n\n", entry->type, entry->base_addr, entry->length, entry->reserved);
        }
    }
}
