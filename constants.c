#include <stdint.h>
#include "constants.h"

/*
Fonts are made up of 5 bytes.
The first 4 bits (so half a byte) of each byte represents the pixels that should be turned on.
So F is drawn by:
0xF0 (1111 0000) ----
0x80 (1000 0000) -
0xF0 (1111 0000) ----
0x80 (1000 0000) -
0x80 (1000 0000) -
*/
const uint8_t c_font_set[C_FONT_SET_SIZE] =
{
    0x60, 0x90, 0x90, 0x90, 0x60, // 0
	0x20, 0x60, 0x20, 0x20, 0x70, // 1
	0xF0, 0x10, 0xF0, 0x80, 0xF0, // 2
	0xF0, 0x10, 0xF0, 0x10, 0xF0, // 3
	0x90, 0x90, 0xF0, 0x10, 0x10, // 4
	0xF0, 0x80, 0xF0, 0x10, 0xF0, // 5
	0xF0, 0x80, 0xF0, 0x90, 0xF0, // 6
	0xF0, 0x10, 0x20, 0x40, 0x40, // 7
	0xF0, 0x90, 0xF0, 0x90, 0xF0, // 8
	0xF0, 0x90, 0xF0, 0x10, 0xF0, // 9
	0xF0, 0x90, 0xF0, 0x90, 0x90, // A
	0xE0, 0x90, 0xE0, 0x90, 0xE0, // B
	0xF0, 0x80, 0x80, 0x80, 0xF0, // C
	0xE0, 0x90, 0x90, 0x90, 0xE0, // D
	0xF0, 0x80, 0xF0, 0x80, 0xF0, // E
	0xF0, 0x80, 0xF0, 0x80, 0x80  // F
};
const uint8_t c_font_offset = 0x50;
const uint16_t c_operations_per_cycle = 2;
const uint16_t c_ram_offset = 0x200;
const int c_target_fps = 60;