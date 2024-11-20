#ifndef EMULATOR_SERVICES_H
#define EMULATOR_SERVICES_H

#include <stdbool.h>
#include <stdint.h>
#include "constants.h"

// Services that the emulator provides to the platform layer.

struct emulator // TODO: put in optimal order to minimise padding
{
    uint8_t memory[C_MEMORY_SIZE];
    int8_t delay_timer;
    int8_t sound_timer;
    uint8_t general_purpose_registers[C_GENERAL_REGISTERS_SIZE];
    uint8_t x;
    uint8_t y;
    uint8_t n;
    uint16_t i;
    uint16_t pc;
    uint16_t instruction;
    uint16_t nnn;
    char display[C_DISPLAY_WIDTH * C_DISPLAY_HEIGHT];
    uint8_t nn;
    uint8_t first_byte;
    uint8_t second_byte;
    uint8_t opcode;
    uint16_t instruction_count; // resets every second
    int_least64_t instrtuctions_start_time_ms;
    int_least64_t delay_timer_start_time_ms;
    uint16_t hz;
};

struct emulator_button_state
{
    bool is_down;
    bool was_down;
};

struct emulator_keyboard_input
{
    union
    {
        struct emulator_button_state buttons[16];
        struct
        {
            struct emulator_button_state numeric_1;
            struct emulator_button_state numeric_2;
            struct emulator_button_state numeric_3;
            struct emulator_button_state numeric_4;
            struct emulator_button_state Q;
            struct emulator_button_state W;
            struct emulator_button_state E;
            struct emulator_button_state R;
            struct emulator_button_state A;
            struct emulator_button_state S;
            struct emulator_button_state D;
            struct emulator_button_state F;
            struct emulator_button_state Z;
            struct emulator_button_state X;
            struct emulator_button_state C;
            struct emulator_button_state V;
        };
    };
};

struct emulator_offscreen_buffer
{
    int height;
    void *memory;
    int pitch;
    int width;
};

struct emulator_sound_output_buffer
{
    int samples_per_second;
    int sample_count;
    int16_t *samples;
};

void
emulator_load_rom(struct emulator *emulator);

void
emulator_output_sound(struct emulator_sound_output_buffer *buffer);

void
emulator_process_opcode
(
    struct emulator_offscreen_buffer *buffer,
    struct emulator_sound_output_buffer *sound_buffer,
    struct emulator_keyboard_input *input,
    struct emulator *emulator
);

#endif