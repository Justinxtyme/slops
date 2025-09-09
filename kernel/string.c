#include "string.h"

int custom_strlen(char *s) {
    int length = 0;
    while (s[length] != '\0') {
        length++;
    }
    return length;
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

