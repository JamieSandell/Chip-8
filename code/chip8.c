#include "chip8.h"

void
emulator_output_sound(struct emulator_sound_output_buffer *sound_buffer)
{
    local_persist f32 t_sine;
    s16 tone_volume = 3000;
    int tone_hz = 256;
    int wave_period = sound_buffer->samples_per_second / tone_hz;
    s16 *sample_out = sound_buffer->samples;
    
    for (int sample_index = 0; sample_index < sound_buffer->sample_count; ++sample_index)
    {
        f32 sine_value = sinf(t_sine);
        s16 sample_value = (s16)(sine_value * tone_volume);
        *sample_out++ = sample_value;
        *sample_out++ = sample_value;
        t_sine += (2.0f * Pi32 * 1.0f) / (f32)wave_period;
    }
}

void
emulator_update_and_render(struct emulator_offscreen_buffer *buffer, struct emulator_sound_output_buffer *sound_buffer)
{
    int x_offset = 0;
    int y_offset = 0;
    render_weird_gradient(buffer, x_offset, y_offset);
}

internal void
render_weird_gradient(struct emulator_offscreen_buffer *buffer, int x_offset, int y_offset)
{
    u8 *row = (u8 *)buffer->memory;
    
    for (int y = 0; y < buffer->height; ++y)
    {
        u32 *pixel = (u32 *)row;
        
        for (int x = 0; x < buffer->width; ++x)
        {
            u8 red = 0;
            u8 green = (u8)(y + y_offset);
            u8 blue = (u8)(x + x_offset);
            
            *pixel++ = red << 16 | green << 8 | blue; // << 0
        }
        
        row += buffer->pitch;
    }
}