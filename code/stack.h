#include <stdbool.h>
#include <stdint.h>

#ifndef STACK_H
#define STACK_H

internal bool
is_empty(void);

internal bool
is_full(void);

internal uint16_t
pop(void);

internal void
push(uint16_t value);

#endif