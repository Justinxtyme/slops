#include "kbd.h"
#include <stdint.h>
#include "shell.h"
#include "serial.h"
#include <stdbool.h>
#include "assertf.h"

#define KBD_DATA_PORT   0x60
#define KBD_STATUS_PORT 0x64
#define KBD_CMD_PORT    0x64

#define KBD_CMD_SET_LEDS 0xED
#define KBD_CMD_ENABLE   0xF4

// Bit masks for LEDs
#define LED_SCROLL 0x01
#define LED_NUM    0x02
#define LED_CAPS   0x04

static uint8_t capslock_state = 0;
static uint8_t numlock_state = 0;
static uint8_t scrolllock_state = 0;

static void kb_wait_write(void) {
    while (inb(KBD_STATUS_PORT) & 0x02); // wait until input buffer clear
}

static void kb_wait_read(void) {
    while (!(inb(KBD_STATUS_PORT) & 0x01)); // wait until output buffer full
}

static void kb_set_leds(uint8_t mask) {
    kb_wait_write();
    outb(KBD_DATA_PORT, KBD_CMD_SET_LEDS); // tell keyboard: set LEDs
    kb_wait_write();
    outb(KBD_DATA_PORT, mask); // send LED bitmask
}

// Flush any pending bytes from the controller
static void kb_flush(void) {
    while (inb(KBD_STATUS_PORT) & 0x01) {
        (void)inb(KBD_DATA_PORT);
    }
}

void kbd_init(void) {
    kb_flush(); // clear any junk from buffer

    // Force known LED state: all off
    capslock_state = 0;
    numlock_state = 0;
    scrolllock_state = 0;
    kb_set_leds(0);

    // Enable scanning so keyboard starts sending key events
    kb_wait_write();
    outb(KBD_DATA_PORT, KBD_CMD_ENABLE);

    // Optional: print debug info
    sfprint("Keyboard initialized. Caps=%d Num=%d Scroll=%d\n",
            capslock_state, numlock_state, scrolllock_state);
}




// keyboard state struct for shell
KBD kbd;


//Initialize struct to 0
void init_kbd_state(void) {
    kbd.lshift   = 0;
    kbd.rshift   = 0;
    kbd.lctrl    = 0;
    kbd.rctrl    = 0;
    kbd.lalt     = 0;
    kbd.ralt     = 0;
    kbd.capslock = 0;
    kbd.numlock  = 0;
}

// returns true if atleast 1 shift is pressed, and the capslock is off
bool cap_true(void) {
     return (kbd.lshift || kbd.rshift) ^ kbd.capslock;
}

// For numbers/symbols: Shift only
bool symbol_shift(void) {
    return (kbd.lshift || kbd.rshift);
}

// takes a scan code and processes it into its proper ascii based 
//on keyboard state
uint8_t scancode2ascii(uint8_t key) {
    //sfprint("Key = %8\n", key);
    uint8_t out = 255;
    switch (key) {
        case 1:
            // ESC
            break;

        case 2: // 1 / !
            if (!symbol_shift()) {
                out = '1';
            } else {
                out = '!';
            }
            break;

        case 3: // 2 / @
            if (!symbol_shift()) {
                out = '2';
            } else {
                out = '@';
            }
            break;

        case 4: // 3 / #
            if (!symbol_shift()) {
                out = '3';
            } else {
                out = '#';
            }
            break;

        case 5: // 4 / $
            if (!symbol_shift()) {
                out = '4';
            } else {
                out = '$';
            }
            break;

        case 6: // 5 / %
            if (!symbol_shift()) {
                out = '5';
            } else {
                out = '%';
            }
            break;

        case 7: // 6 / ^
            if (!symbol_shift()) {
                out = '6';
            } else {
                out = '^';
            }
            break;

        case 8: // 7 / &
            if (!symbol_shift()) {
                out = '7';
            } else {
                out = '&';
            }
            break;

        case 9: // 8 / *
            if (!symbol_shift()) {
                out = '8';
            } else {
                out = '*';
            }
            break;

        case 10: // 9 / (
            if (!symbol_shift()) {
                out = '9';
            } else {
                out = '(';
            }
            break;

        case 11: // 0 / )
            if (!symbol_shift()) {
                out = '0';
            } else {
                out = ')';
            }
            break;

        case 12: // - / _
            if (!symbol_shift()) {
                out = '-';
            } else {
                out = '_';
            }
            break;

        case 13: // = / +
            if (!symbol_shift()) {
                out = '=';
            } else {
                out = '+';
            }
            break;

        case 14:
            out = '\b';
            break;

        case 15:
            // Tab
            break;

        case 16: // q / Q
            if (!cap_true()) {
                out = 'q';
            } else {
                out = 'Q';
            }
            break;

        case 17: // w / W
            if (!cap_true()) {
                out = 'w';
            } else {
                out = 'W';
            }
            break;

        case 18: // e / E
            if (!cap_true()) {
                out = 'e';
            } else {
                out = 'E';
            }
            break;

        case 19: // r / R
            if (!cap_true()) {
                out = 'r';
            } else {
                out = 'R';
            }
            break;

        case 20: // t / T
            if (!cap_true()) {
                out = 't';
            } else {
                out = 'T';
            }
            break;

        case 21: // y / Y
            if (!cap_true()) {
                out = 'y';
            } else {
                out = 'Y';
            }
            break;

        case 22: // u / U
            if (!cap_true()) {
                out = 'u';
            } else {
                out = 'U';
            }
            break;

        case 23: // i / I
            if (!cap_true()) {
                out = 'i';
            } else {
                out = 'I';
            }
            break;

        case 24: // o / O
            if (!cap_true()) {
                out = 'o';
            } else {
                out = 'O';
            }
            break;

        case 25: // p / P
            if (!cap_true()) {
                out = 'p';
            } else {
                out = 'P';
            }
            break;

        case 26: // [ / {
            if (!cap_true()) {
                out = '[';
            } else {
                out = '{';
            }
            break;

        case 27: // ] / }
            if (!cap_true()) {
                out = ']';
            } else {
                out = '}';
            }
            break;

        case 28:
            out = '\n';
            break;

        case 29:
            // CTRL
            break;

        case 30: // a / A
            if (!cap_true()) {
                out = 'a';
            } else {
                out = 'A';
            }
            break;

        case 31: // s / S
            if (!cap_true()) {
                out = 's';
            } else {
                out = 'S';
            }
            break;

        case 32: // d / D
            if (!cap_true()) {
                out = 'd';
            } else {
                out = 'D';
            }
            break;

        case 33: // f / F
            if (!cap_true()) {
                out = 'f';
            } else {
                out = 'F';
            }
            break;

        case 34: // g / G
            if (!cap_true()) {
                out = 'g';
            } else {
                out = 'G';
            }
            break;

        case 35: // h / H
            if (!cap_true()) {
                out = 'h';
            } else {
                out = 'H';
            }
            break;

        case 36: // j / J
            if (!cap_true()) {
                out = 'j';
            } else {
                out = 'J';
            }
            break;

        case 37: // k / K
            if (!cap_true()) {
                out = 'k';
            } else {
                out = 'K';
            }
            break;

        case 38: // l / L
            if (!cap_true()) {
                out = 'l';
            } else {
                out = 'L';
            }
            break;

        case 39: // ; / :
            if (!cap_true()) {
                out = ';';
            } else {
                out = ':';
            }
            break;

        case 40: // ' / "
            if (!cap_true()) {
                out = '\'';
            } else {
                out = '"';
            }
            break;

        case 41: // ` / ~
            if (!cap_true()) {
                out = '`';
            } else {
                out = '~';
            }
            break;

        case 42: // left shift
            kbd.lshift = 1;
            out = 255;
            //sfprint("170: Shift LR = %8, %8\n", kbd.lshift, kbd.rshift);
            break;  
        
        case 43:
            if (!cap_true()) {
                out = '\\';
            } else {
                out = '|';
            }
            break;
            
        case 44: // z / Z
            if (!cap_true()) {
                out = 'z';
            } else {
                out = 'Z';
            }
            break;

        case 45: // x / X
            if (!cap_true()) {
                out = 'x';
            } else {
                out = 'X';
            }
            break;

        case 46: // c / C
            if (!cap_true()) {
                out = 'c';
            } else {
                out = 'C';
            }
            break;

        case 47: // v / V
            if (!cap_true()) {
                out = 'v';
            } else {
                out = 'V';
            }
            break;

        case 48: // b / B
            if (!cap_true()) {
                out = 'b';
            } else {
                out = 'B';
            }
            break;

        case 49: // n / N
            if (!cap_true()) {
                out = 'n';
            } else {
                out = 'N';
            }
            break;

        case 50: // m / M
            if (!cap_true()) {
                out = 'm';
            } else {
                out = 'M';
            }
            break;

        case 51: // , / <
            if (!cap_true()) {
                out = ',';
            } else {
                out = '<';
            }
            break;

        case 52: // . / >
            if (!cap_true()) {
                out = '.';
            } else {
                out = '>';
            }
            break;

        case 53: // / / ?
            if (!cap_true()) {
                out = '/';
            } else {
                out = '?';
            }
            break;

        case 54: // right shift
            kbd.rshift = 1;
            out = 255;
            //sfprint("170: Shift LR = %8, %8\n", kbd.lshift, kbd.rshift);
            break;  

        case 55:
            // Print Screen
            break;

        case 56:
            // Alt
            break;

        case 57: // spacebar
            out = ' ';
            break;

        case 58: // caps lock
            if (kbd.capslock == 0) {
                kbd.capslock = 1;
                out = 255;
            } else {
                kbd.capslock = 0;
                out = 255;
            }
            break;

        case 59:
            // F1
            break;

        case 60:
            // F2
            break;

        case 61:
            // F3
            break;

        case 62:
            // F4
            break;

        case 63:
            // F5
            break;

        case 64:
            // F6
            break;

        case 65:
            // F7
            break;

        case 66:
            // F8
            break;

        case 67:
            // F9
            break;

        case 68:
            // F10
            break;

        case 69:
            // Num Lock
            break;

        case 70:
            // Scroll Lock
            break;

        case 71:
            // Home (Keypad 7)
            break;

        case 72:
            // Up (Keypad 8)
            break;

        case 73:
            // Page Up (Keypad 9)
            break;

        case 74:
            out = '-';
            break;

        case 75:
            // Left (Keypad 4)
            break;

        case 76:
            // Center (Keypad 5)
            break;

        case 77:
            // Right (Keypad 6)
            break;

        case 78:
            out = '+';
            break;

        case 79:
            // End (Keypad 1)
            break;

        case 80:
            // Down (Keypad 2)
            break;

        case 81:
            // Page Down (Keypad 3)
            break;

        case 82:
            // Insert
            break;

        case 83:
            // Delete
            break;
        
        case 170:
            kbd.lshift = 0;
            out = 255;
            //sfprint("170: Shift LR = %8, %8\n", kbd.lshift, kbd.rshift);
            break;
        
        case 182:
            kbd.rshift = 0;
            out = 255;
            //sfprint("182: Shift LR = %8, %8\n", kbd.lshift, kbd.rshift);
            break;        
         
        default:
            // if (key > 83 && key < 170) {
            //     out = 255; // or leave out untouched
            //     break;
            // }
            break;
        }
    
    //sfprint("OUT:Shift LR = %8, %8\n", kbd.lshift, kbd.rshift);
    return out;
}
