#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include "shared.h"
#include "stack.h"

#define DISPLAY_WIDTH 64
#define DISPLAY_HEIGHT 32
#define MEMORY_SIZE 4096
#define MAX_MESSAGE_SIZE 1024

int main(int argc, char *argv[])
{
    char message[MAX_MESSAGE_SIZE];
    snprintf(message, MAX_MESSAGE_SIZE, "Error: Incorrect number of command-line arguments. Expected <program name> <filename>\n");
    terminate(argc != 2, message);

    FILE *fpr = fopen(argv[1], "rb");
    snprintf(message, MAX_MESSAGE_SIZE, "Error: failed to open %s for reading.\n", argv[1]);
    terminate(fpr == NULL, message);

    snprintf(message, MAX_MESSAGE_SIZE, "Error: failed to seek to the end of %s\n", argv[1]);
    terminate(fseek(fpr, 0L, SEEK_END), message);

    long file_size = ftell(fpr);
    snprintf(message, MAX_MESSAGE_SIZE, "Error: failed to get the file size of %s\n", argv[1]);
    terminate(file_size == -1L, message);

    snprintf(message, MAX_MESSAGE_SIZE, "Error: failed to seek to the beginning of %s\n", argv[1]);
    terminate(fseek(fpr, 0L, SEEK_SET), message);

    char memory[MEMORY_SIZE];
    snprintf(message, MAX_MESSAGE_SIZE, "Error: failed to read the contents of %s\n", argv[1]);
    terminate(fread(memory, 1, (size_t)file_size, fpr) != (size_t)file_size, message);

    char display[DISPLAY_WIDTH * DISPLAY_HEIGHT];
    int16_t pc;
    int16_t i;

    return EXIT_SUCCESS;
}

