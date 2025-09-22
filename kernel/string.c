#include "string.h"
#include <stdint.h>
#include "types.h"
#include "mem.h"

int cst_strcmp(char *str1, char *str2) {
    int len1 = custom_strlen(str1);
    int len2 = custom_strlen(str2);
    
    if (len1 != len2) {
       //sfprint("cst_strcmp: strings are not equal\n");
       return 0;
    }
    
    char *p1 = str1;
    char *p2 = str2;  
    int cnt  = 0;
    while (*p1) {
      sfprint("p1[%8] = %c  p2[%8] = %c\n", cnt, *p1, cnt, *p2);  
      if (*p1 != *p2) {
            return 0;
        } else {
            ++p1;
            ++p2;
            ++cnt;
        }
    }
    //sfprint("cst_strcmp: strings are equal\n");
    return 1;
}

int str_eq(const char *a, const char *b) {
    while (*a && *b) {
        if (*a != *b) return 0;
        a++;
        b++;
    }
    return (*a == '\0' && *b == '\0');
}

int custom_strlen(char *s) {
    int length = 0;
    while (s[length] != '\0') {
        length++;
    }
    return length;
}

char* strdupe(const char* str) {
    size_t len = custom_strlen(str);
    char* dup = thralloc(len + 1); // or thrcalloc(len + 1, sizeof(char)) for zeroed
    if (!dup) return NULL;

    for (size_t i = 0; i < len; i++) {
        dup[i] = str[i];
    }
    dup[len] = '\0';

    return dup;
}



_Bool space(char c) {
    //return ((char) & (" "));
    if (c == " ") {
        return 1;
    } else {
        return 0;
    }
}


// Function to reverse the string
void reverse_string(char *str) {
    int length = custom_strlen(str);
    int start = 0;
    int end = length - 1;
    char temp;

    while (start < end) {
        // Swap characters
        temp = str[start];
        str[start] = str[end];
        str[end] = temp;

        // Move pointers
        start++;
        end--;
    }
    str[length] = '\0'; 
}

void int_2_string(int val, char* buff) {
    int i = 0;
    int dec = 0;
    _Bool is_neg = 0;
    if (val < 0) {
        is_neg = 1;
        val = -val;
        buff[i++] = '-';
    }
    
    while (val > 0) {
        if (val >= 10) {
            dec = val % 10;
            dec += '0';
            buff[i++] = dec;
            val /= 10;
            continue;
        } else {
            val += '0';
            buff[i++] = val;
            break;
        }
    }
    buff[i] = '\0';
    reverse_string(buff);  
} 

int isprint(uint8_t c) {
    return (c >= 0x20 && c <= 0x7E);
}

void print_ascii(char *buffer, size_t len){
    for (int i = 0; i < len; ++i) {

    } 
}

char val2ascii(uint8_t val) {
    switch (val) {
        case 10: return '\n';
        case 32: return ' ';  
        case 33: return '!';  
        case 34: return '"';  
        case 35: return '#';
        case 36: return '$';
        case 37: return '%';
        case 38: return '&';
        case 39: return '\'';
        case 40: return '(';
        case 41: return ')';
        case 42: return '*';
        case 43: return '+';
        case 44: return ',';
        case 45: return '-';
        case 46: return '.';
        case 47: return '/';
        case 48: return '0';
        case 49: return '1';
        case 50: return '2';
        case 51: return '3';
        case 52: return '4';
        case 53: return '5';
        case 54: return '6';
        case 55: return '7';
        case 56: return '8';
        case 57: return '9';
        case 58: return ':';
        case 59: return ';';
        case 60: return '<';
        case 61: return '=';
        case 62: return '>';
        case 63: return '?';
        case 64: return '@';
        case 65: return 'A';
        case 66: return 'B';
        case 67: return 'C';
        case 68: return 'D';
        case 69: return 'E';
        case 70: return 'F';
        case 71: return 'G';
        case 72: return 'H';
        case 73: return 'I';
        case 74: return 'J';
        case 75: return 'K';
        case 76: return 'L';
        case 77: return 'M';
        case 78: return 'N';
        case 79: return 'O';
        case 80: return 'P';
        case 81: return 'Q';
        case 82: return 'R';
        case 83: return 'S';
        case 84: return 'T';
        case 85: return 'U';
        case 86: return 'V';
        case 87: return 'W';
        case 88: return 'X';
        case 89: return 'Y';
        case 90: return 'Z';
        case 91: return '[';
        case 92: return '\\';
        case 93: return ']';
        case 94: return '^';
        case 95: return '_';
        case 96: return '`';
        case 97: return 'a';
        case 98: return 'b';
        case 99: return 'c';
        case 100: return 'd';
        case 101: return 'e';
        case 102: return 'f';
        case 103: return 'g';
        case 104: return 'h';
        case 105: return 'i';
        case 106: return 'j';
        case 107: return 'k';
        case 108: return 'l';
        case 109: return 'm';
        case 110: return 'n';
        case 111: return 'o';
        case 112: return 'p';
        case 113: return 'q';
        case 114: return 'r';
        case 115: return 's';
        case 116: return 't';
        case 117: return 'u';
        case 118: return 'v';
        case 119: return 'w';
        case 120: return 'x';
        case 121: return 'y';
        case 122: return 'z';
        case 123: return '{';
        case 124: return '|';
        case 125: return '}';
        case 126: return '~';
        default: return '?'; // fallback for unsupported values
    }
}

