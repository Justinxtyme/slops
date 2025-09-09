//vga.h
#ifndef VGA_H
#define VGA_H

#include <stdint.h>

void print(const char* str);

void itoa(unsigned int value, char* buffer);

void litoa(unsigned long int value, char* buffer);

void llitoa(unsigned long long int value, char* buffer);

void log_tag_to_vga(uint32_t type, uint32_t size);

#endif