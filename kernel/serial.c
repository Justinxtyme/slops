#include "serial.h"
#include "string.h"
#include "vga.h"

#include <stdint.h>

//////////////////////////////////////////////////////////////////
//SERIAL OUTPUT TO LINUX ////////////////////////////////////////
////////////////////////////////////////////////////////////////   

#define COM1_PORT 0x3F8 // serial port for printing to host terminal

uint8_t inb(uint16_t port) {
    uint8_t ret;
    __asm__ volatile ("inb %1, %0" : "=a"(ret) : "Nd"(port));
    return ret;
}

void outb(uint16_t port, uint8_t val) {
    asm volatile ("outb %0, %1" : : "a"(val), "Nd"(port));
}

void serial_init() {
    outb(COM1_PORT + 1, 0x00);    // Disable interrupts
    outb(COM1_PORT + 3, 0x80);    // Enable DLAB
    outb(COM1_PORT + 0, 0x03);    // Set baud rate divisor (low byte)
    outb(COM1_PORT + 1, 0x00);    // High byte
    outb(COM1_PORT + 3, 0x03);    // 8 bits, no parity, one stop bit
    outb(COM1_PORT + 2, 0xC7);    // Enable FIFO, clear them
    outb(COM1_PORT + 4, 0x0B);    // IRQs enabled, RTS/DSR set
}

void serial_write_char(char c) {
    while (!(inb(COM1_PORT + 5) & 0x20)); // Wait until transmit buffer is empty
    outb(COM1_PORT, c);
}

void serial_write(const char* s) {
    while (*s) {
        serial_write_char(*s++);
    }
}

/* Make the table file-scope so it’s undeniably in .rodata */
static const char HEX[16] = "0123456789ABCDEF";


/* Tiny helper: write one nibble (0..15) */
static inline void serial_write_hex_nibble(uint8_t x) {
    serial_write_char(HEX[x & 0xF]);
}


/* Prints exactly 2 hex digits */
void serial_write_hex8(uint8_t v) {
    serial_write_hex_nibble((uint8_t)(v >> 4));
    serial_write_hex_nibble((uint8_t)(v      ));
}

/* Prints exactly 4 hex digits, high byte first */
void serial_write_hex16(uint16_t v) {
    serial_write_hex8((uint8_t)(v >> 8));
    serial_write_hex8((uint8_t)(v     ));
}

/* Prints exactly 8 hex digits, big-endian bytes */
void serial_write_hex32(uint32_t v) {
    serial_write_hex8((uint8_t)(v >> 24));
    serial_write_hex8((uint8_t)(v >> 16));
    serial_write_hex8((uint8_t)(v >> 8 ));
    serial_write_hex8((uint8_t)(v      ));
}

/* Prints exactly 16 hex digits, big-endian bytes */
void serial_write_hex64(uint64_t v) {
    uint32_t hi = (uint32_t)(v >> 32);
    uint32_t lo = (uint32_t)(v      );
    serial_write_hex32(hi);
    serial_write_hex32(lo);
}

void u32tohex(uint32_t val, char* buff) {
    const char* hex_chars = "0123456789ABCDEF";
    for (int i = 0; i < 8; i++) {
        buff[i] = hex_chars[(val >> (28 - i * 4)) & 0xF];
    }
    buff[8] = '\0';
}

void u8tohex(uint8_t val, char* out) {
    const char* hex = "0123456789ABCDEF";
    out[0] = hex[(val >> 4) & 0xF];
    out[1] = hex[val & 0xF];
    out[2] = '\0';
}




void format_sfprint(const char* fmt, va_list args) {
    const char *p = fmt;

    while (*p) {
        if (*p != '%') {
            serial_write_char(*p++);
            continue;
            
        }

        p++; // skip '%'

        if (*p == '\0') break; // stray '%' at end of string

        switch (*p) {
            case 'd': { // integer
                int val = va_arg(args, int);
                char buff[32];
                itoa(val,buff);
                //int_2_string(val, buff);
                serial_write(buff);
                break;
            }
            case '8': { //prints 64 bit num. still works for all unsigned sizes (8,16,32)
                uint64_t val = va_arg(args, uint64_t);
                char buff[32];
                llitoa(val,buff);
                //int_2_string(val, buff);
                serial_write(buff);
                break;
            }
            case 's': { //string
                const char *sval = va_arg(args, const char*);
                serial_write(sval);
                break;
            }
            case 'x': {
                uint32_t val = va_arg(args, uint32_t);
                char buff[32];
                u32tohex(val, buff); 
                serial_write(buff);
                break;
            }
            case 'h': { // hex byte
                uint8_t val = va_arg(args, int); // promoted to int
                char buff[3];
                u8tohex(val, buff); // you'll write this
                serial_write(buff);
                break;
            }

            case '%': {
                serial_write_char('%'); // handle literal %%
                break;
            }
            default:
                serial_write_char('?'); // unknown format
                break;
        }

        p++; // move past format specifier
    }
}

void sfprint(const char* str, ...) {
    va_list args;
    va_start(args, str);
    format_sfprint(str, args);
    va_end(args);
}


