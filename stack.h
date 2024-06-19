#include <stdbool.h>
#include <stdint.h>

#ifndef STACK_H
#define STACK_H

#define STACK_SIZE 1024

static bool is_empty(void);
static bool is_full(void);

int16_t pop(void);
void push(int16_t value);

#endif