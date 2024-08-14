#include <stdio.h>
#include "constants.h"
#include "globals.h"
#include "stack.h"

uint16_t contents[C_MAX_STACK_SIZE];
int top = -1;

internal bool
is_empty(void)
{
    return top == -1;
}

internal bool
is_full(void)
{
    return top == C_MAX_STACK_SIZE - 1;
}

internal uint16_t
pop(void)
{
    // TODO: Assert stack is not empty
    
    return contents[top--];
}

internal void
push(uint16_t value)
{
    // TODO: Assert stack is not full
    
    contents[top++] = value;
}