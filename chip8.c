#include <debugapi.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include "chip8.h"
#include "constants.h"
#include "emulator_services.h"
#include "globals.h"
#include "platform_services.h"
#include "stack.h"

void
emulator_load_rom(struct emulator *emulator)
{
    if (emulator->memory)
    {
        platform_free_file_memory(emulator->memory);
    }
    
    struct read_file_result result = platform_read_entire_file(".\\data\\4-flags.ch8"); // TODO: Don't hardcode
    
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
    srand((unsigned int)time(0));
    emulator->start_time_ms = platform_get_milliseconds_now();
}

void
emulator_update_and_render(struct emulator_offscreen_buffer *buffer,
                           struct emulator_sound_output_buffer *sound_buffer,
                           struct emulator_keyboard_input *input,
                           struct emulator *emulator)
{    
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
    emulator->instruction = (uint16_t)((emulator->first_byte << 8) | emulator->second_byte);
    emulator->x = emulator->first_byte & 0x0F; // second nibble
    emulator->y = (emulator->second_byte >> 4) & 0x0F; // 3rd nibble
    emulator->n = emulator->second_byte & 0x0F; // 4th nibble
    emulator->nn = emulator->second_byte;
    emulator->nnn = (uint16_t)(emulator->x << 8); // second nibble
    emulator->nnn |= (uint16_t)(emulator->y << 4); // third nibble
    emulator->nnn = (uint16_t)(emulator->nnn | emulator->n); // 4th nibble
    emulator->opcode = (emulator->first_byte >> 4) & 0x0F;
    emulator->pc = (uint16_t)(emulator->pc + 2);
    ++emulator->instruction_count;
    
    for (int key = 0; key < 16; ++key)
    {
        if (input->buttons[key].is_down)
        {
            snprintf(global_message, C_MAX_MESSAGE_SIZE, "%d is down.\n", key);
            OutputDebugStringA(global_message);
        }
    }
    
    switch (emulator->opcode)
    {
        case 0:
        {
            switch (emulator->nnn)
            {
                case 0x0E0:
                {
                    memset(emulator->display, 0, sizeof(emulator->display) / sizeof(emulator->display[0]));
                    
                    uint8_t *destination_row = (uint8_t *)buffer->memory;
                    uint8_t *source_row = (uint8_t *)emulator->display; // TODO: should this not need the cast?
                    
                    for (int y = 0; y < C_DISPLAY_HEIGHT; ++y)
                    {
                        uint8_t *destination_pixel = destination_row;
                        uint8_t *source_pixel = source_row;
                        
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
                    snprintf(global_message, C_MAX_MESSAGE_SIZE, "Error decoding the rest of the 0 instruction.\n");
                    OutputDebugStringA(global_message);
                } break;
            } break;
        }
        case 1:
        {
            //OutputDebugStringA("goto NNN\n");
            emulator->pc = emulator->nnn;
        } break;
        case 2:
        {
            OutputDebugStringA("*(0xNNN)()\n");
            push(emulator->pc);
            emulator->pc = emulator->nnn;
        } break;
        case 3:
        {
            if (emulator->general_purpose_registers[emulator->x] == emulator->nn)
            {
                OutputDebugStringA("if (Vx == NN)\n");
                emulator->pc = (uint16_t)(emulator->pc + 2);
            }
        } break;
        case 4:
        {
            if (emulator->general_purpose_registers[emulator->x] != emulator->nn)
            {
                OutputDebugStringA("if (Vx != NN)\n");
                emulator->pc = (uint16_t)(emulator->pc + 2);
            }
        } break;
        case 5:
        {
            if (emulator->general_purpose_registers[emulator->x] == emulator->general_purpose_registers[emulator->y])
            {
                OutputDebugStringA("if (Vx == Vy)\n");
                emulator->pc = (uint16_t)(emulator->pc + 2);
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
            emulator->general_purpose_registers[emulator->x] = (uint8_t)(emulator->general_purpose_registers[emulator->x] + emulator->nn);
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
                    uint16_t vx_vy = (uint16_t)(emulator->general_purpose_registers[emulator->x] + emulator->general_purpose_registers[emulator->y]);
                    emulator->general_purpose_registers[emulator->x] = (uint8_t)vx_vy;
                    emulator->general_purpose_registers[0xF] = vx_vy > UCHAR_MAX ? 1 : 0;
                    
                } break;
                case 5:
                {
                    OutputDebugStringA("Vx -= Vy\n");
                    
                    emulator->general_purpose_registers[emulator->x] = (uint8_t)(emulator->general_purpose_registers[emulator->x] - emulator->general_purpose_registers[emulator->y]);
                    emulator->general_purpose_registers[0xF] = emulator->general_purpose_registers[emulator->x] > emulator->general_purpose_registers[emulator->y] ? 1 : 0;
                } break;
                case 6:
                {
                    OutputDebugStringA("Vx >>= 1\n");
                    emulator->general_purpose_registers[0xF] = emulator->general_purpose_registers[emulator->x] & 0x1u;
                    emulator->general_purpose_registers[emulator->x] = (emulator->general_purpose_registers[emulator->x] >> 1);
                } break;
                case 7:
                {
                    OutputDebugStringA("Vx = Vy - Vx\n");
                    uint8_t *x = (uint8_t *)&emulator->general_purpose_registers[emulator->x];
                    uint8_t *y = (uint8_t *)&emulator->general_purpose_registers[emulator->y];
                    
                    *x = (uint8_t)(*y - *x);
                    emulator->general_purpose_registers[0xF] = *x > *y ? 1 : 0; // Do Vx - Vy and store that value in Vx. If Vx is greater than Vy, then Vf = 1, else Vf =0 
                } break;
                case 0xE:
                {
                    OutputDebugStringA("Vx <<= 1\n");
                    emulator->general_purpose_registers[0xF] = (uint8_t)(emulator->general_purpose_registers[emulator->x] & 0x80u);
                    emulator->general_purpose_registers[emulator->x] = (uint8_t)(emulator->general_purpose_registers[emulator->x] << 1);
                } break;
                default:
                {
                    snprintf(global_message, C_MAX_MESSAGE_SIZE, "Error decoding the rest of the 8 instruction.\n");
                    OutputDebugStringA(global_message);
                } break;
            }
        } break;
        case 9:
        {
            OutputDebugStringA("if (Vx != Vy)\n");
            if (emulator->general_purpose_registers[emulator->x] != emulator->general_purpose_registers[emulator->y])
            {
                OutputDebugStringA("if (Vx != Vy)\n");
                emulator->pc = (uint16_t)(emulator->pc + 2);
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
            emulator->pc = (uint16_t)(emulator->general_purpose_registers[0] + emulator->nnn);
        } break;
        case 0xC:
        {
            OutputDebugStringA("Vx = rand() & NN\n");
            emulator->general_purpose_registers[emulator->x] = (uint8_t)(rand() % 255 & emulator->nn);
        } break;
        case 0xD:
        {
            OutputDebugStringA("draw(Vx, Vy, N)\n");
            uint8_t x = emulator->general_purpose_registers[emulator->x] % C_DISPLAY_WIDTH;
            uint8_t y = emulator->general_purpose_registers[emulator->y] % C_DISPLAY_HEIGHT;
            uint8_t *display_pixel = (uint8_t *)(emulator->display);
            emulator->general_purpose_registers[0xF] = 0;
            uint8_t *sprite_row = &emulator->memory[emulator->i];
            
            for (int row = 0; row < emulator->n; ++row)
            {
                uint8_t sprite_byte = *sprite_row;
                uint8_t *display_row = (uint8_t *)(emulator->display + ((y + row) * C_DISPLAY_WIDTH));
                
                for (int column = 0; column < 8; ++column)
                {
                    uint8_t sprite_pixel = (uint8_t)(sprite_byte & (0x80 >> column));
                    uint8_t *display_pixel = display_row + x + column;
                    
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
            
            uint8_t *destination_row = (uint8_t *)buffer->memory; //TODO: Refactor to method
            uint8_t *source_row = (uint8_t *)(emulator->display);
            
            for (int y = 0; y < C_DISPLAY_HEIGHT; ++y)
            {
                uint8_t *destination_pixel = destination_row;
                uint8_t *source_pixel = source_row;
                
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
        case 0xE:
        {
            switch (emulator->nn)
            {
                case 0x9E:
                {
                    OutputDebugString("if (key() == Vx)\n");
                    
                    if (input->buttons[emulator->general_purpose_registers[emulator->x]].is_down)
                    {
                        emulator->pc = (uint16_t)(emulator->pc + 2);
                    }
                } break;
                case 0xA1:
                {
                    OutputDebugString("if (key() != Vx)\n");
                    
                    if (!input->buttons[emulator->general_purpose_registers[emulator->x]].is_down)
                    {
                        emulator->pc = (uint16_t)(emulator->pc + 2);
                    }
                } break;
                default:
                {
                    snprintf(global_message, C_MAX_MESSAGE_SIZE, "Error decoding the rest of the E instruction.\n");
                    OutputDebugStringA(global_message);
                } break;
            } break;
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
                        if (input->buttons[key].was_down && !input->buttons[key].is_down)
                        {
                            key_pressed = true;
                            break;
                        }
                    }
                    
                    if (key_pressed)
                    {
                        emulator->general_purpose_registers[emulator->x] = (uint8_t)key;
                    }
                    else
                    {
                        emulator->pc = (uint16_t)(emulator->pc - 2);
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
                case 0x1E:
                {
                    OutputDebugStringA("I += Vx\n");
                    emulator->i = (uint16_t)(emulator->i + emulator->general_purpose_registers[emulator->x]);
                    emulator->general_purpose_registers[0xF] = emulator->i > 0x0FFF ? 1 : 0;
                } break;
                case 0x29:
                {
                    OutputDebugStringA("I = sprite_addr[Vx]\n");
                    emulator->i = (uint16_t)(emulator->memory[c_font_offset] + (emulator->general_purpose_registers[emulator->x] * 5));
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
                    
                    for (int i = 0; i <= emulator->x; ++i)
                    {
                        emulator->memory[emulator->i + i] = emulator->general_purpose_registers[i];
                    }

                    //emulator->i = (uint16_t)(emulator->i + emulator->x + 1); // TODO: Create a toggle for this "quirk"
                } break;
                case 0x65:
                {
                    OutputDebugStringA("reg_load(Vx, &I)\n");
                    
                    for (int i = 0; i <= emulator->x; ++i)
                    {
                        emulator->general_purpose_registers[i] = emulator->memory[emulator->i + i];
                    }

                    //emulator->i = (uint16_t)(emulator->i + emulator->x + 1); // TODO: Create a toggle for this "quirk"
                } break;
                default:
                {
                    snprintf(global_message, C_MAX_MESSAGE_SIZE, "Error decoding the rest of the F instruction.\n");
                    OutputDebugStringA(global_message);
                } break;
            }
        } break;
        default:
        {
            snprintf(global_message, C_MAX_MESSAGE_SIZE, "Opcode (first nibble) %1X at memory address %d not implemented.\n", emulator->pc, emulator->opcode);
            OutputDebugStringA(global_message);
        } break;
    }
}