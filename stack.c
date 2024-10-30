#include <stdbool.h>
#include <stdio.h>
#include "globals.h"
#include "constants.h"
#include "stack.h"
#include "terminate.h"

static int16_t contents[C_MAX_STACK_SIZE];
static int top = -1;

static bool is_empty(void);
static bool is_full(void);

static bool is_empty(void)
{
    return top == -1;
}

static bool is_full(void)
{
    return top == C_MAX_STACK_SIZE - 1;
}

int16_t pop(void)
{
    snprintf(global_message, C_MAX_STACK_SIZE, "Error: can't pop an empty stack.\n");
    terminate(is_empty(), global_message);

    return contents[top--];
}

void push(int16_t value)
{
    snprintf(global_message, C_MAX_STACK_SIZE, "Error: can't push to a full stack.\n");
    terminate(is_full(), global_message);

    contents[++top] = value;
}