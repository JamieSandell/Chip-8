/* date = July 15th 2024 7:11 am */

#ifndef CHIP8_H
#define CHIP8_H

#if DEBUG
#define Assert(Expression) if (!(Expression)) { *(int) *0 = 0; }
#else
#define Assert(Expression)
#endif

struct emulator_button_state
{
    s32 half_transition_count;
    b32 ended_down;
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

// TODO: Services that the platform layer provides to the emulator.

// NOTE: Services that the emulator provides to the platform layer.
void
emulator_output_sound(struct emulator_sound_output_buffer *buffer);

void
emulator_update_and_render(struct emulator_offscreen_buffer *buffer, struct emulator_sound_output_buffer *sound_buffer, struct emulator_keyboard_input *input);

internal void
render_weird_gradient(struct emulator_offscreen_buffer *buffer, int x_offset, int y_offset);

#endif //CHIP8_H
