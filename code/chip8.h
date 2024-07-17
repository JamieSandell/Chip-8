/* date = July 15th 2024 7:11 am */

#ifndef CHIP8_H
#define CHIP8_H

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
    s16 samples;
};

// TODO: Services that the platform layer provides to the emulator.

// NOTE: Services that the emulator provides to the platform layer.
void
emulator_output_sound(struct emulator_sound_output_buffer *buffer);

void
emulator_update_and_render(struct emulator_offscreen_buffer *buffer, struct emulator_sound_output_buffer *sound_buffer);

internal void
render_weird_gradient(struct emulator_offscreen_buffer *buffer, int x_offset, int y_offset);

#endif //CHIP8_H
