#include <string.h>
#include "chip8.h"

internal void
emulator_init(struct emulator *emulator)
{
    if (emulator->memory)
    {
        platform_free_file_memory(emulator->memory);
    }
    
    struct read_file_result result = platform_read_entire_file("..\\data\\IBM Logo.ch8"); // TODO: Don't hardcode
    
    if (!result.contents)
    {
        // TODO: Logging, error file contents is NULL
        return; // TODO: return failure?
    }
    
    memcpy(emulator->memory + c_ram_offset, result.contents, (size_t)result.contents_size);
    
    for (size_t i = 0; i < C_FONT_SET_SIZE; ++i)
    {
        emulator->memory[c_font_offset + i] = c_font_set[i];
    }
    
    emulator->pc = c_ram_offset;
}

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
emulator_update_and_render(struct emulator_offscreen_buffer *buffer,
                           struct emulator_sound_output_buffer *sound_buffer,
                           struct emulator_keyboard_input *input,
                           struct emulator *emulator)
{
    
    local_persist int x_offset = 0;
    
    if (input->numeric_1.ended_down)
    {
        x_offset += 1;
    }
    
    int y_offset = 0;
    emulator_output_sound(sound_buffer);
    
    local_persist int first_run = 1;
    
    if(first_run)
    {
        first_run = 0;
        //render_weird_gradient(buffer, x_offset, y_offset);
        struct read_file_result result = platform_read_entire_file("W:\\data\\IBM Logo.ch8");
        
        if (result.contents)
        {
            memcpy(emulator->memory, result.contents, result.contents_size);
        }
    }
    
    emulator->first_byte = emulator->memory[emulator->pc];
    emulator->second_byte = emulator->memory[emulator->pc + 1];
    emulator->instruction = (emulator->first_byte << 8) | emulator->second_byte;
    emulator->x = emulator->first_byte & 0x0F; // second nibble
    emulator->y = (emulator->second_byte >> 4) & 0x0F; // 3rd nibble
    emulator->n = emulator->second_byte & 0x0F; // 4th nibble
    emulator->nn = emulator->second_byte;
    emulator->nnn = (uint16_t)emulator->x << 8; // second nibble
    emulator->nnn |= (uint16_t)emulator->y << 4; // third nibble
    emulator->nnn |= (uint16_t)emulator->n; // 4th nibble
    emulator->opcode = (emulator->first_byte >> 4) & 0x0F;
    emulator->pc += 2;
    
    switch (emulator->opcode)
    {
        case 0:
        {
            switch (emulator->nnn)
            {
                case 0x0E0:
                {
                    memset(emulator->display, 0, sizeof(emulator->display) / sizeof(emulator->display[0]));
                    
                    u8 *destination_row = (u8 *)buffer->memory;
                    u8 *source_row = emulator->display;
                    
                    for (int y = 0; y < C_DISPLAY_HEIGHT; ++y)
                    {
                        u8 *destination_pixel = destination_row;
                        u8 *source_pixel = source_row;
                        
                        for (int x = 0; x < C_DISPLAY_WIDTH; ++x, *source_pixel++)
                        {
                            for (int pixel = 0; pixel < 4; ++pixel)
                            {
                                *destination_pixel++ = *source_pixel;
                            }
                        }
                        
                        destination_row += buffer->pitch;
                        source_row += C_DISPLAY_WIDTH;
                    }
                    
                    OutputDebugStringA("disp_clear()\n");
                } break;
                case 0x0EE:
                {
                    OutputDebugStringA("return\n");
                } break;
                default:
                {
                    snprintf(g_message, C_MAX_MESSAGE_SIZE, "Error decoding the rest of the 0 instruction.\n");
                    OutputDebugStringA(g_message);
                } break;
            } break;
        }
        case 1:
        {
            OutputDebugStringA("goto NNN\n");
            emulator->pc = emulator->nnn;
        } break;
        case 6:
        {
            OutputDebugStringA("Vx = NN\n");
            emulator->general_purpose_registers[emulator->n] = emulator->nn;
        } break;
        case 7:
        {
            OutputDebugStringA("Vx += NN\n");
            emulator->general_purpose_registers[emulator->n] += emulator->nn;
        } break;
        case 0xA:
        {
            OutputDebugStringA("I = NNN\n");
            emulator->i = emulator->nnn;
        } break;
        case 0xD:
        {
            OutputDebugStringA("draw(Vx, Vy, N)\n");
            u8 x = emulator->general_purpose_registers[emulator->x] % C_DISPLAY_WIDTH;
            u8 y = emulator->general_purpose_registers[emulator->y] % C_DISPLAY_HEIGHT;
            u8 *display_pixel = emulator->display;
            display_pixel += y * C_DISPLAY_WIDTH;
            display_pixel += x;
            emulator->general_purpose_registers[0X0F] = 0;
            u8 *sprite_row = &emulator->memory[emulator->i];
            
            for (int y = 0; y < emulator->n; ++y)
            {
                u8 *sprite_pixel = sprite_row;
                
                for (int x = 0; x < 8; ++x)
                {
                    if(*sprite_pixel && *display_pixel)
                    {
                        *display_pixel = 0;
                        emulator->general_purpose_registers[0X0F] = 1;
                    }
                    else if(!(*sprite_pixel) && !(*display_pixel))
                    {
                        *display_pixel = 255;
                    }
                    
                    ++display_pixel;
                    ++sprite_pixel;
                }
                
                sprite_row += 8;
            }
            
            u8 *destination_row = (u8 *)buffer->memory; //TODO: Refactor to method
            u8 *source_row = emulator->display;
            
            for (int y = 0; y < C_DISPLAY_HEIGHT; ++y)
            {
                u8 *destination_pixel = destination_row;
                u8 *source_pixel = source_row;
                
                for (int x = 0; x < C_DISPLAY_WIDTH; ++x, *source_pixel++)
                {
                    for (int pixel = 0; pixel < 4; ++pixel)
                    {
                        *destination_pixel++ = *source_pixel;
                    }
                }
                
                destination_row += buffer->pitch;
                source_row += C_DISPLAY_WIDTH;
            }
        } break;
        default:
        {
            snprintf(g_message, C_MAX_MESSAGE_SIZE, "Opcode (first nibble) %1X at memory address %d not implemented.\n", emulator->pc, emulator->opcode);
            OutputDebugStringA(g_message);
        } break;
    }
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