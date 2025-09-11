//string.h
#ifndef STRING_H
#define STRING_H

#include <stdint.h>

int custom_strlen(char *s);

void reverse_string(char *str);

void int_2_string(int val, char* buff);

int isprint(uint8_t c);

#endif