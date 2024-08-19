#include <limits.h>
#include <string.h>
#include <time.h>
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
    srand(time(0));
    emulator->start_time_ms = platform_get_milliseconds_now();
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
    
    local_persist int x_offset = 0; // TODO: Remove local_persist
    
    if (input->numeric_1.ended_down)
    {
        x_offset += 1;
    }
    
    int y_offset = 0;
    emulator_output_sound(sound_buffer);
    
    local_persist int first_run = 1; // TODO: Remove local_persist
    
    if (first_run) // TODO: Remove
    {
        first_run = 0;
        //render_weird_gradient(buffer, x_offset, y_offset);
        struct read_file_result result = platform_read_entire_file("W:\\data\\IBM Logo.ch8");
        
        if (result.contents)
        {
            memcpy(emulator->memory, result.contents, result.contents_size);
        }
    }
    
    if (emulator->delay_timer > 0)
    {
        --emulator->delay_timer;
    }
    
    if (emulator->sound_timer > 0)
    {
        --emulator->sound_timer;
    }
    
    if (emulator->sound_timer < 0)
    {
        OutputDebugStringA("Beep!");
        // TODO: Play sound
    }
    
    int_least64_t elapsed_time_ms = platform_get_milliseconds_now() - emulator->start_time_ms;
    
    // TODO: Remove hardcoding
    if (elapsed_time_ms <= 1000LL && emulator->instruction_count >= c_instructions_per_second)
    {
        return;
    }
    
    if (elapsed_time_ms > 1000LL) // TODO: Remove hardcoding
    {
        emulator->instruction_count = 0;
        emulator->start_time_ms = platform_get_milliseconds_now();
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
    ++emulator->instruction_count;
    
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
                    emulator->pc = pop();
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
        case 2:
        {
            OutputDebugStringA("*(0xNNN)()\n");
            emulator->pc = emulator->nnn;
            push(emulator->pc);
        } break;
        case 3:
        {
            if (emulator->general_purpose_registers[emulator->x] == emulator->nn)
            {
                OutputDebugStringA("if (Vx == NN)\n");
                emulator->pc += 2;
            }
        } break;
        case 4:
        {
            if (emulator->general_purpose_registers[emulator->x] != emulator->nn)
            {
                OutputDebugStringA("if (Vx != NN)\n");
                emulator->pc += 2;
            }
        } break;
        case 5:
        {
            if (emulator->general_purpose_registers[emulator->x] == emulator->general_purpose_registers[emulator->y])
            {
                OutputDebugStringA("if (Vx == Vy)\n");
                emulator->pc += 2;
            }
        } break;
        case 6: 
        {
            OutputDebugStringA("Vx = NN\n");
            emulator->general_purpose_registers[emulator->x] = emulator->nn;
        } break;
        case 7:
        {
            OutputDebugStringA("Vx += NN\n");
            emulator->general_purpose_registers[emulator->x] += emulator->nn;
        } break;
        case 8:
        {
            switch (emulator->n)
            {
                case 0:
                {
                    OutputDebugStringA("Vx = Vy\n");
                    emulator->general_purpose_registers[emulator->x] = emulator->general_purpose_registers[emulator->y];
                } break;
                case 1:
                {
                    OutputDebugStringA("Vx |= Vy\n");
                    emulator->general_purpose_registers[emulator->x] |= emulator->general_purpose_registers[emulator->y];
                } break;
                case 2:
                {
                    OutputDebugStringA("Vx &= Vy\n");
                    emulator->general_purpose_registers[emulator->x] &= emulator->general_purpose_registers[emulator->y];
                } break;
                case 3:
                {
                    OutputDebugStringA("Vx ^= Vy\n");
                    emulator->general_purpose_registers[emulator->x] ^= emulator->general_purpose_registers[emulator->y];
                } break;
                case 4:
                {
                    OutputDebugStringA("Vx += Vy\n");
                    uint16_t vx_vy = (uint16_t)emulator->general_purpose_registers[emulator->x] + (uint16_t)emulator->general_purpose_registers[emulator->y];
                    emulator->general_purpose_registers[emulator->x] += emulator->general_purpose_registers[emulator->y];
                    emulator->general_purpose_registers[0xF] = vx_vy> UCHAR_MAX ? 1 : 0;
                    
                } break;
                case 5:
                {
                    OutputDebugStringA("Vx -= Vy\n");
                    uint8_t *x = &emulator->general_purpose_registers[emulator->x];
                    uint8_t *y = &emulator->general_purpose_registers[emulator->y];
                    
                    emulator->general_purpose_registers[0xF] = *x > *y ? 1 : 0;
                    *x -= *y;
                } break;
                case 6:
                {
                    OutputDebugStringA("Vx >>= 1\n");
                    uint8_t *x = &emulator->general_purpose_registers[emulator->x];
                    emulator->general_purpose_registers[0xF] = *x & 0x1;
                    *x >>= 1;
                } break;
                case 7:
                {
                    OutputDebugStringA("Vx = Vy - Vx\n");
                    uint8_t *x = &emulator->general_purpose_registers[emulator->x];
                    uint8_t *y = &emulator->general_purpose_registers[emulator->y];
                    
                    emulator->general_purpose_registers[0xF] = *x >= *y ? 1 : 0;
                    *x = *y - *x;
                } break;
                case 0xE:
                {
                    OutputDebugStringA("Vx <<= 1\n");
                    uint8_t *x = &emulator->general_purpose_registers[emulator->x];
                    emulator->general_purpose_registers[0xF] = *x & 0x80;
                } break;
                default:
                {
                    snprintf(g_message, C_MAX_MESSAGE_SIZE, "Error decoding the rest of the 8 instruction.\n");
                    OutputDebugStringA(g_message);
                } break;
            }
        } break;
        case 9:
        {
            OutputDebugStringA("if (Vx != Vy)\n");
            if (emulator->general_purpose_registers[emulator->x] != emulator->general_purpose_registers[emulator->y])
            {
                OutputDebugStringA("if (Vx != Vy)\n");
                emulator->pc += 2;
            }
        } break;
        case 0xA:
        {
            OutputDebugStringA("I = NNN\n");
            emulator->i = emulator->nnn;
        } break;
        case 0xB:
        {
            OutputDebugStringA("PC = V0 + NNN\n");
            emulator->pc = emulator->general_purpose_registers[0] + emulator->nnn;
        } break;
        case 0xC:
        {
            OutputDebugStringA("Vx = rand() & NN\n");
            emulator->general_purpose_registers[emulator->x] = rand() % 255 & emulator->nn;
        } break;
        case 0xD:
        {
            OutputDebugStringA("draw(Vx, Vy, N)\n");
            u8 x = emulator->general_purpose_registers[emulator->x] % C_DISPLAY_WIDTH;
            u8 y = emulator->general_purpose_registers[emulator->y] % C_DISPLAY_HEIGHT;
            u8 *display_pixel = emulator->display;
            emulator->general_purpose_registers[0xF] = 0;
            u8 *sprite_row = &emulator->memory[emulator->i];
            
            for (int row = 0; row < emulator->n; ++row)
            {
                u8 sprite_byte = *sprite_row;
                u8 *display_row = emulator->display + ((y + row) * C_DISPLAY_WIDTH);
                
                for (int column = 0; column < 8; ++column)
                {
                    u8 sprite_pixel = sprite_byte & (0x80 >> column);
                    u8 *display_pixel = display_row + x + column;
                    
                    if(sprite_pixel && *display_pixel)
                    {
                        *display_pixel = 0;
                        emulator->general_purpose_registers[0xF] = 1;
                    }
                    else if(sprite_pixel && !(*display_pixel))
                    {
                        *display_pixel = 0xFF;
                    }
                    
                    /*if ((x + column) % C_DISPLAY_WIDTH == 0)
                    {
                        break;
                    }*/
                    
                    ++display_pixel;
                }
                
                ++sprite_row;
                
                /*if ((y + row) % C_DISPLAY_HEIGHT == 0)
                {
                    break;
                }*/
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
        case 0xF:
        {
            switch (emulator->nn)
            {
                case 0x07:
                {
                    OutputDebugStringA("Vx = get_delay()\n");
                    emulator->general_purpose_registers[emulator->x] = emulator->delay_timer;
                } break;
                case 0x0A: // NOTE: block until a key has been released (not just held down)
                {
                    OutputDebugStringA("Vx = get_key()\n");
                    bool key_pressed = false;
                    int key;
                    
                    for (key = 0; key < 16; ++key)
                    {
                        if (input->buttons[key].ended_down)
                        {
                            key_pressed = true;
                            break;
                        }
                    }
                    
                    if (key_pressed)
                    {
                        emulator->general_purpose_registers[emulator->x] = key;
                    }
                    else
                    {
                        emulator->pc -= 2;
                    }
                } break;
                case 0x15:
                {
                    OutputDebugStringA("delay_timer(Vx)\n");
                    emulator->delay_timer = emulator->general_purpose_registers[emulator->x];
                } break;
                case 0x18:
                {
                    OutputDebugStringA("sound_timer(Vx)\n");
                    emulator->sound_timer = emulator->general_purpose_registers[emulator->x];
                } break;
                case 0x29:
                {
                    OutputDebugStringA("I = sprite_addr[Vx]\n");
                    emulator->i = emulator->memory[c_font_offset] + (emulator->general_purpose_registers[emulator->x] * 5);
                } break;
                case 0x33:
                {
                    OutputDebugStringA("set_BCD(Vx *(I+0) = BCD(3);*(I+1) = BCD(2);*(I+2) = BCD(1);\n");
                    uint8_t vx = emulator->general_purpose_registers[emulator->x];
                    emulator->memory[emulator->i] = vx / 100;
                    emulator->memory[emulator->i + 1] = (vx / 10) % 10;
                    emulator->memory[emulator->i + 2] = (vx % 100) % 10;
                } break;
                case 0x55:
                {
                    OutputDebugStringA("reg_dump(Vx, &I)\n");
                    
                    for (int reg = 0, x = emulator->general_purpose_registers[emulator->x];  reg < x; ++reg)
                    {
                        emulator->memory[emulator->i++] = emulator->general_purpose_registers[reg];
                    }
                } break;
                case 0x65:
                {
                    OutputDebugStringA("reg_load(Vx, &I)\n");
                    
                    for (int reg = 0, vx = emulator->general_purpose_registers[emulator->x]; reg < vx; ++reg)
                    {
                        emulator->general_purpose_registers[reg] = emulator->memory[emulator->i++];
                    }
                } break;
                default:
                {
                    snprintf(g_message, C_MAX_MESSAGE_SIZE, "Error decoding the rest of the F instruction.\n");
                    OutputDebugStringA(g_message);
                } break;
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