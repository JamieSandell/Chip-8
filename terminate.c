#include <stdio.h>
#include <stdlib.h>
#include "terminate.h"

void terminate(bool condition, const char *message)
{
    if (condition)
    {
        fprintf(stderr, "%s", message);
        exit(EXIT_FAILURE);
    }
}