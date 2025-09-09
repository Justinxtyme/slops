// serial.h
#ifndef SERIAL_H
#define SERIAL_H

#include <stdint.h>
#include <stdarg.h>


void serial_init();

void serial_write_char(char c);

void serial_write(const char* s);

uint8_t inb(uint16_t port);

void outb(uint16_t port, uint8_t val);

void serial_write_hex8(uint8_t v);

void serial_write_hex16(uint16_t v);

void serial_write_hex32(uint32_t v);

void serial_write_hex64(uint64_t v);

void sfprint(const char *str, ...);

void format_sfprint(const char* str, va_list args);


#endif