//kbd.h
#ifndef KBD_H
#define KBD_H

#include <stdbool.h>
#include <stdint.h>

#define KBDBUFFSIZE 256
#define KBDBUF_MASK (KBDBUFFSIZE - 1)


typedef struct KbdState {
    bool lshift;
    bool rshift;
    bool lctrl;
    bool rctrl;
    bool lalt;
    bool ralt;
    bool capslock;
    bool numlock;
    // ...
} KBD;


typedef struct {
    volatile uint16_t head;       // producer writes
    volatile uint16_t tail;       // consumer writes
    uint8_t scancodes[KBDBUFFSIZE];
} kbuff_t;


uint8_t scancode2ascii(uint8_t key);

void init_kbd_state(void);

bool cap_true(void);

bool symbol_shift(void);



void kbd_init(void); 

#endif