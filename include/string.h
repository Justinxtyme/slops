//string.h
#ifndef STRING_H
#define STRING_H

#include <stdint.h>
#include "serial.h"

int cst_strcmp(char *str1, char *str2);

int custom_strlen(char *s);

void reverse_string(char *str);

void int_2_string(int val, char* buff);

int isprint(uint8_t c);

char val2ascii(uint8_t val);

#endif