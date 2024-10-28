#ifndef GLOBALS_H
#define GLOBALS_H

#include <stdbool.h>
#include "constants.h"

extern char global_message[C_MAX_MESSAGE_SIZE];
extern bool global_running;

#if DEBUG
#define Assert(Expression) if (!(Expression)) { *(int) *0 = 0; }
#else
#define Assert(Expression)
#endif

#endif