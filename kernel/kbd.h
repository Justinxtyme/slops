//kbd.h
#ifndef KBD_H
#define KBD_H

#include <stdbool.h>


struct KeyboardState {
    bool lshift;
    bool rshift;
    bool lctrl;
    bool rctrl;
    bool lalt;
    bool ralt;
    bool capslock;
    // ...
};


#endif