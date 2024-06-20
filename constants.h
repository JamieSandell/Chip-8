#ifndef CONSTANTS_H
#define CONSTANTS_H

#include <stdint.h>

#define C_DISPLAY_HEIGHT 32
#define C_DISPLAY_WIDTH 64
#define C_FONT_SET_SIZE 80
#define C_MAX_MESSAGE_SIZE 1024
#define C_MAX_STACK_SIZE 1024
#define C_MEMORY_SIZE 1024

extern const uint8_t c_font_set[C_FONT_SET_SIZE];
extern const uint8_t c_font_offset;

#endif