#include "chip8.h"

void
emulator_update_and_render(struct game_offscreen_buffer *buffer)
{
    int x_offset = 0;
    int y_offset = 0;
    render_weird_gradient(buffer, x_offset, y_offset);
}

internal void
render_weird_gradient(struct game_offscreen_buffer *buffer, int x_offset, int y_offset)
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