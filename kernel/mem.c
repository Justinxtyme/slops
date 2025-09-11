#include "mem.h"


void fb_memcpy(uint8_t* dest, const uint8_t* src, size_t count) {
    for (size_t i = 0; i < count; i++) {
        dest[i] = src[i];
    }
}

void fb_memset(uint8_t* dest, uint8_t value, size_t count) {
    for (size_t i = 0; i < count; i++) {
        dest[i] = value;
    }
}