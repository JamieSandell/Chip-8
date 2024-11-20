#ifndef CONSTANTS_H
#define CONSTANTS_H

#include <stdint.h>

#define C_DISPLAY_HEIGHT 32
#define C_DISPLAY_WIDTH 64
#define C_FONT_SET_SIZE 80
#define C_GENERAL_REGISTERS_SIZE 16
#define C_MAX_MESSAGE_SIZE 1024
#define C_MAX_STACK_SIZE 1024
#define C_MEMORY_SIZE 4096

extern const uint8_t c_font_set[C_FONT_SET_SIZE];
extern const uint8_t c_font_offset;
extern const uint16_t c_operations_per_cycle;
extern const uint16_t c_ram_offset;

#endif