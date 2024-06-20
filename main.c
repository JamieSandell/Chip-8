#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include "constants.h"
#include "stack.h"
#include "terminate.h"

int main(int argc, char *argv[])
{
    snprintf(c_message, c_max_message_size, "Error: Incorrect number of command-line arguments. Expected <program name> <filename>\n");
    terminate(argc != 2, c_message);

    FILE *fpr = fopen(argv[1], "rb");
    snprintf(c_message, c_max_message_size, "Error: failed to open %s for reading.\n", argv[1]);
    terminate(fpr == NULL, c_message);

    snprintf(c_message, c_max_message_size, "Error: failed to seek to the end of %s\n", argv[1]);
    terminate(fseek(fpr, 0L, SEEK_END), c_message);

    long file_size = ftell(fpr);
    snprintf(c_message, c_max_message_size, "Error: failed to get the file size of %s\n", argv[1]);
    terminate(file_size == -1L, c_message);

    snprintf(c_message, c_max_message_size, "Error: failed to seek to the beginning of %s\n", argv[1]);
    terminate(fseek(fpr, 0L, SEEK_SET), c_message);

    char memory[MEMORY_SIZE];
    snprintf(g_message, c_max_message_size, "Error: failed to read the contents of %s\n", argv[1]);
    terminate(fread(memory, 1, (size_t)file_size, fpr) != (size_t)file_size, g_message);

    char display[DISPLAY_WIDTH * DISPLAY_HEIGHT];
    int16_t pc;
    int16_t i;
    int8_t delay_timer;
    int8_t sound_timer;
    struct GeneralPurposeRegisters
    {
        int8_t V0;
        int8_t V1;
        int8_t V2;
        int8_t V3;
        int8_t V4;
        int8_t V5;
        int8_t V6;
        int8_t V7;
        int8_t V8;
        int8_t V9;
        int8_t VA;
        int8_t VB;
        int8_t VC;
        int8_t VD;
        int8_t VE;
        int8_t VF;
    } general_purpose_registers;

    return EXIT_SUCCESS;
}

