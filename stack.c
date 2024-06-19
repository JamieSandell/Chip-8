#include "stack.h"

static int top = -1;
static int16_t contents[STACK_SIZE];

bool is_empty(void)
{
    return top == -1;
}

bool is_full(void)
{
    return top == STACK_SIZE - 1;
}

int16_t pop(void)
{
    
}