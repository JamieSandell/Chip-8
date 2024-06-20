#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include "constants.h"
#include "globals.h"
#include "stack.h"
#include "terminate.h"

int main(int argc, char *argv[])
{
    snprintf(g_message, C_MAX_MESSAGE_SIZE, "Error: Incorrect number of command-line arguments. Expected <program name> <filename>\n");
    terminate(argc != 2, g_message);

    FILE *fpr = fopen(argv[1], "rb");
    snprintf(g_message, C_MAX_MESSAGE_SIZE, "Error: failed to open %s for reading.\n", argv[1]);
    terminate(fpr == NULL, g_message);

    snprintf(g_message, C_MAX_MESSAGE_SIZE, "Error: failed to seek to the end of %s\n", argv[1]);
    terminate(fseek(fpr, 0L, SEEK_END), g_message);

    long file_size = ftell(fpr);
    snprintf(g_message, C_MAX_MESSAGE_SIZE, "Error: failed to get the file size of %s\n", argv[1]);
    terminate(file_size == -1L, g_message);

    snprintf(g_message, C_MAX_MESSAGE_SIZE, "Error: failed to seek to the beginning of %s\n", argv[1]);
    terminate(fseek(fpr, 0L, SEEK_SET), g_message);

    uint8_t memory[C_MEMORY_SIZE];
    snprintf(g_message, C_MAX_MESSAGE_SIZE, "Error: failed to read the contents of %s\n", argv[1]);
    terminate(fread(memory + c_ram_offset, 1, (size_t)file_size, fpr) != (size_t)file_size, g_message);

    for (size_t i = 0; i < C_FONT_SET_SIZE; ++i)
    {
        memory[c_font_offset + i] = c_font_set[i];
    }

    char display[C_DISPLAY_WIDTH * C_DISPLAY_HEIGHT];
    uint16_t i;
    int8_t delay_timer;
    int8_t sound_timer;
    int8_t general_purpose_registers[C_GENERAL_REGISTERS_SIZE];

    uint16_t pc = c_ram_offset;
    uint16_t instruction;
    uint8_t x;
    uint8_t y;
    uint8_t n;
    uint8_t nn;
    uint16_t nnn;
    uint8_t first_byte;
    uint8_t second_byte;
    uint8_t opcode;

    do
    {
        first_byte = memory[pc];
        second_byte = memory[pc + 1];
        instruction = (first_byte << 8) | second_byte;
        x = first_byte & 0x0F; // second nibble
        y = (second_byte >> 4) & 0x0F; // 3rd nibble
        n = second_byte & 0x0F; // 4th nibble
        nn = second_byte;
        nnn = (uint16_t)x << 8; // second nibble
        nnn |= (uint16_t)y << 4; // third nibble
        nnn |= (uint16_t)n; // 4th nibble
        opcode = (first_byte >> 4) & 0x0F;
        pc += 2;

        switch (opcode)
        {
            case 0:
            {
                switch (nnn)
                {
                    case 0x0E0:
                    {
                        printf("disp_clear()\n");
                        break;
                    }
                    case 0x0EE:
                    {
                        printf("return\n");
                        break;
                    }
                    default:
                    {
                        snprintf(g_message, C_MAX_MESSAGE_SIZE, "Error decoding the rest of the 0 instruction.\n");
                        terminate(true, g_message);
                    }
                }
                break;
            }
            case 1:
            {
                printf("goto NNN\n");
                pc = nnn;
                break;
            }
            case 6:
            {
                printf("Vx = NN\n");
                general_purpose_registers[n] = nn;
                break;
            }
            case 7:
            {
                printf("Vx += NN\n");
                general_purpose_registers[n] += nn;
                break;
            }
            case 0xA:
            {
                printf("I = NNN\n");
                i = nnn;
                break;
            }
            case 0xD:
            {
                printf("draw(Vx, Vy, N)\n");
                break;
            }
            default:
            {
                snprintf(g_message, C_MAX_MESSAGE_SIZE, "Opcode (first nibble) %1X at memory address %d not implemented.\n", pc, opcode);
                terminate(true, g_message);
            }
        }

    } while (true);
    

    return EXIT_SUCCESS;
}