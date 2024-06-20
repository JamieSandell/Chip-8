#include <stdio.h>
#include "constants.h"
#include "stack.h"
#include "terminate.h"

static int16_t contents[c_max_stack_size];
static int top = -1;

static bool is_empty(void);
static bool is_full(void);

static bool is_empty(void)
{
    return top == -1;
}

static bool is_full(void)
{
    return top == c_max_stack_size - 1;
}

int16_t pop(void)
{
    snprintf(g_message, c_max_message_size, "Error: can't pop an empty stack.\n");
    terminate(is_empty(), g_message);

    return contents[top--];
}

void push(int16_t value)
{
    snprintf(g_message, c_max_message_size, "Error: can't push to a full stack.\n");
    terminate(is_full(), g_message);

    contents[top++] = value;
}