#include "kbd.h"
#include <stdint.h>





//bool shift_active = false;

uint8_t scancode2ascii(uint8_t key) {
    uint8_t out;
    switch (key) {
        case 1:
            // ESC
            break;

        case 2:
            out = '1';
            break;

        case 3:
            out = '2';
            break;

        case 4:
            out = '3';
            break;

        case 5:
            out = '4';
            break;

        case 6:
            out = '5';
            break;

        case 7:
            out = '6';
            break;

        case 8:
            out = '7';
            break;

        case 9:
            out = '8';
            break;

        case 10:
            out = '9';
            break;

        case 11:
            out = '0';
            break;

        case 12:
            out = '-';
            break;

        case 13:
            out = '=';
            break;

        case 14:
            out = '\b';
            break;

        case 15:
            // Tab
            break;

        case 16:
            out = 'Q';
            break;

        case 17:
            out = 'W';
            break;

        case 18:
            out = 'E';
            break;

        case 19:
            out = 'R';
            break;

        case 20:
            out = 'T';
            break;

        case 21:
            out = 'Y';
            break;

        case 22:
            out = 'U';
            break;

        case 23:
            out = 'I';
            break;

        case 24:
            out = 'O';
            break;

        case 25:
            out = 'P';
            break;

        case 26:
            out = '[';
            break;

        case 27:
            out = ']';
            break;

        case 28:
            out = '\n';
            break;

        case 29:
            // CTRL
            break;

        case 30:
            out = 'A';
            break;

        case 31:
            out = 'S';
            break;

        case 32:
            out = 'D';
            break;

        case 33:
            out = 'F';
            break;

        case 34:
            out = 'G';
            break;

        case 35:
            out = 'H';
            break;

        case 36:
            out = 'J';
            break;

        case 37:
            out = 'K';
            break;

        case 38:
            out = 'L';
            break;

        case 39:
            out = ';';
            break;

        case 40:
            out = '\'';
            break;

        case 41:
            out = '`';
            break;

        case 42:
            // Left Shift
            break;

        case 43:
            out = '\\';
            break;

        case 44:
            out = 'Z';
            break;

        case 45:
            out = 'X';
            break;

        case 46:
            out = 'C';
            break;

        case 47:
            out = 'V';
            break;

        case 48:
            out = 'B';
            break;

        case 49:
            out = 'N';
            break;

        case 50:
            out = 'M';
            break;

        case 51:
            out = ',';
            break;

        case 52:
            out = '.';
            break;

        case 53:
            out = '/';
            break;

        case 54:
            // Right Shift
            break;

        case 55:
            // Print Screen
            break;

        case 56:
            // Alt
            break;

        case 57:
            out = ' ';
            break;

        case 58:
            // Caps Lock
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
    }
    if (key > 83) {
        out = '1';
    }
    return out;
}
