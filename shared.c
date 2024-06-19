#include <stdlib.h>
#include "shared.h"

void terminate(bool condition, const char *message)
{
    if (condition)
    {
        fprintf(stderr, "%s", message);
        exit(EXIT_FAILURE);
    }
}