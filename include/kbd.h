//kbd.h
#ifndef KBD_H
#define KBD_H

#include <stdbool.h>
#include <stdint.h>

#define KBDBUFFSIZE 128


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
    uint8_t head;
    uint8_t tail;
    uint8_t scancodes[KBDBUFFSIZE];
    
} kbuff_t;

uint8_t scancode2ascii(uint8_t key);

void init_kbd_state(void);

bool cap_true(void);

bool symbol_shift(void);



void kbd_init(void); 

#endif