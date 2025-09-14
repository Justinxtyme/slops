#include "mem.h"
#include "serial.h"
#include "multiboot.h"


#define MAX_PAGES 1048576 // for 4 GiB of RAM
#define HEAP_SIZE 4096
uint8_t page_bitmap[MAX_PAGES / 8]; // 128 KiB


extern uint8_t* _heap_start[];

Chunk *free_head;

void mem_init(){
    sfprint("mem_init: Entered mem_init\n");
    sfprint("mem_init: Heap start: 0x%x\n", (uint32_t)(uintptr_t)_heap_start);
    free_head = (Chunk*)_heap_start;
    sfprint("mem_init: free head chunked\n");
    free_head->size = HEAP_SIZE - sizeof(Chunk);
    sfprint("mem_init: free_head size: %8\n", free_head->size);
    free_head->is_free = 1;
    free_head->next = ((void *)0);
    sfprint("mem_init: free_head is_free: %8\n", free_head->is_free);
    sfprint("mem_init: free_head next: %8\n", free_head->next);
}


void* thralloc(size_t size) { // Alloc a block of mem from the heap of at least 'size' bytes
sfprint("Allocating %8 bytes\n", size); 
    Chunk *current = free_head; //scan from the first free chunk 
    Chunk *prev = ((void *)0); // Keep track of the previous chunk (currently unused here, initialized to NULL)
    // Loop through linked list of chunks until out
    while (current) { 
        if ((current->is_free) && (current->size >= size)) { // if chunk is free and chunky enough
            if (current->size >= size + sizeof(Chunk)) { // if the chunk is too chunky, split
                Chunk *new_chunk = (Chunk *)((uint8_t *)(current + 1) + size); // Calculate address of new chunk header after alloc'd space
                new_chunk->size = current->size - size - sizeof(Chunk); // Set new chunk's size to leftover space after alloc
                new_chunk->is_free = 1; // Mark as free
                new_chunk->next = current->next; // Link new chunk into list after current chunk
                current->size = size; // Shrink current chunk to the requested size
                current->next = new_chunk; // Link current chunk to new chunk
            }
            current->is_free = 0; // Mark the current chunk as used
            return (void*)(current + 1); // Return a pointer to the memory just after the chunk header (usable space)
        }
        current = current->next; // Move to the next chunk in the list
    }
    return (void*)0; // No suitable chunk found, return NULL
}




size_t thralloc_total() {
    size_t total = 0;
    Chunk* current = free_head; // scan from first free chunk

    while (current) {
        if (!current->is_free) { // while chunk is used
            total += current->size + sizeof(Chunk); // add size of used chunk to total
        }
        current = current->next; // walk to the next chunk
    }
    sfprint("Total memory allocated: %8\n", total);
    return total;
}


void tfree(void *ptr) { // Free a previously allocated block
    if (ptr == (void*)0) return; // Ignore NULL frees
    Chunk *chunk = ((Chunk*)ptr) - 1; // Step back from the payload to get the chunk header
    sfprint("Freeing %8 bytes\n", chunk->size);
    chunk->is_free = 1; // Mark this chunk as free

    // Coalesce with next chunk if it's free
    if (chunk->next && chunk->next->is_free) {
        chunk->size += sizeof(Chunk) + chunk->next->size; // Absorb next chunk's space
        chunk->next = chunk->next->next; // Skip over the absorbed chunk
    }

    // Coalesce with previous chunk if it's free
    Chunk *current = free_head;
    Chunk *prev = (void*)0;
    while (current && current != chunk) { // Walk list to find the chunk
        prev = current;
        current = current->next;
    }
    if (prev && prev->is_free) {
        prev->size += sizeof(Chunk) + chunk->size; // Merge into previous
        prev->next = chunk->next; // Skip over current chunk
    }
}




int cralloc(size_t size) {
    Chunk *current = free_head;
    Chunk *prev = ((void *)0);
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
