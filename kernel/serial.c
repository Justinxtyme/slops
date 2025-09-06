#include "serial.h"

#include <stdint.h>
//////////////////////////////////////////////////////////////////
//SERIAL OUTPUT TO LINUX ////////////////////////////////////////
////////////////////////////////////////////////////////////////   

#define COM1_PORT 0x3F8 // serial port for printing to host terminal

static inline uint8_t inb(uint16_t port) {
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
