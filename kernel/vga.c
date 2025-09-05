#include "vga.h"

volatile char* video = (volatile char*)0xB8000;
int cursor = 0;

void print(const char* str) {
    while (*str) {
        video[cursor++] = *str;      // ASCII character
        video[cursor++] = 0x07;      // Attribute: light gray on black
    }
}


void itoa(unsigned int value, char* buffer) {
    char* p = buffer;
    if (value == 0) {
        *p++ = '0';
    } else {
        char temp[10];
        int i = 0;
        while (value > 0) {
            temp[i++] = '0' + (value % 10);
            value /= 10;
        }
        while (i--) {
            *p++ = temp[i];
        }
    }
    *p = '\0';
}

void log_tag_to_vga(uint32_t type, uint32_t size) {
    char buffer[32];

    print("[MB2] Tag type: ");
    itoa(type, buffer);
    print(buffer);

    print(" | size: ");
    itoa(size, buffer);
    print(buffer);

    print("\n");
}

