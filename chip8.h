#ifndef CHIP8_H
#define CHIP8_H

#include <stdbool.h>
#include <stdint.h>
#include "constants.h"

#if DEBUG
#define Assert(Expression) if (!(Expression)) { *(int) *0 = 0; }
#else
#define Assert(Expression)
#endif

uint8_t
decode_key(uint8_t key);

uint8_t
encode_key(uint8_t key);

inline uint32_t
safe_truncate_uint64(uint64_t value)
{
    Assert(value < 0xFFFFFFFF); // nothing in the upper 32 bits?
    return (uint32_t)value;
}

#endif