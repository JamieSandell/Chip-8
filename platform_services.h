#include <stdint.h>

#ifndef PLATFORM_SERVICES_H
#define PLATFORM_SERVICES_H

//Services that the platform layer provides to the emulator.
struct read_file_result
{
    uint32_t contents_size;
    void *contents;
};

void
platform_free_file_memory(void *memory);

int_least64_t
platform_get_milliseconds_now(void);

struct read_file_result
platform_read_entire_file(const char *filename);

#endif