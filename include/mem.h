#ifndef MEM_H
#define MEM_H

#include <stdint.h>
#include <stddef.h> 

void fb_memcpy(uint8_t* dest, const uint8_t* src, size_t count);

void fb_memset(uint8_t* dest, uint8_t value, size_t count);

#endif