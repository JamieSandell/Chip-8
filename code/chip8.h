/* date = July 15th 2024 7:11 am */

#ifndef CHIP8_H
#define CHIP8_H

#if DEBUG
#define Assert(Expression) if (!(Expression)) { *(int) *0 = 0; }
#else
#define Assert(Expression)
#endif

inline u32
safe_truncate_uint64(u64 value)
{
    Assert(value < 0xFFFFFFFF); // nothing in the upper 32 bits?
    return (u32)value;
}

// TODO: Services that the platform layer provides to the emulator.
struct read_file_result
{
    u32 contents_size;
    void *contents;
};

internal void
platform_free_file_memory(void *memory);

internal int_least64_t
platform_get_milliseconds_now(void);

internal struct read_file_result
platform_read_entire_file(const char *filename);

// NOTE: Services that the emulator provides to the platform layer.

struct emulator // TODO: put in optimal order to minimise padding
{
    uint8_t memory[C_MEMORY_SIZE];
    int8_t delay_timer;
    int8_t sound_timer;
    int8_t general_purpose_registers[C_GENERAL_REGISTERS_SIZE];
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
    int_least64_t start_time_ms;
};

struct emulator_button_state
{
    s32 half_transition_count;
    b32 ended_down;
    b32 is_down;
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
    s16 *samples;
};

internal void
emulator_init(struct emulator *emulator);

internal void
emulator_output_sound(struct emulator_sound_output_buffer *buffer);

internal void
emulator_update_and_render(struct emulator_offscreen_buffer *buffer, struct emulator_sound_output_buffer *sound_buffer, struct emulator_keyboard_input *input, struct emulator *emulator);

internal void
render_weird_gradient(struct emulator_offscreen_buffer *buffer, int x_offset, int y_offset);

#endif //CHIP8_H
