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

uint8_t
decode_key(uint8_t key)
{
    uint8_t value;

    switch (key)
    {
        case 0x1:
        {
            value = 0;
        } break;
        case 0x2:
        {
            value = 1;
        } break;
        case 0x3:
        {
            value = 2;
        } break;
        case 0xC:
        {
            value = 3;
        } break;
        case 0x4:
        {
            value = 4;
        } break;
        case 0x5:
        {
            value = 5;
        } break;
        case 0x6:
        {
            value = 6;
        } break;
        case 0xD:
        {
            value = 7;
        } break;
        case 0x7:
        {
            value = 8;
        } break;
        case 0x8:
        {
            value = 9;
        } break;
        case 0x9:
        {
            value = 10;
        } break;
        case 0xE:
        {
            value = 11;
        } break;
        case 0xA:
        {
            value = 12;
        } break;
        case 0x0:
        {
            value = 13;
        } break;
        case 0xB:
        {
            value = 14;
        } break;
        case 0xF:
        {
            value = 15;
        } break;
        default:
        {
            // TODO: error handling
        } break;
    }

    return value;
}

void
emulator_load_rom(struct emulator *emulator)
{
    if (emulator->memory)
    {
        platform_free_file_memory(emulator->memory);
    }
    
    struct read_file_result result = platform_read_entire_file(".\\data\\5-quirks.ch8"/*".\\data\\6-keypad.ch8"*/); // TODO: Don't hardcode
    
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
    emulator->hz = 1320;
    emulator->timer_start_time_ms = platform_get_milliseconds_now();
}

void
emulator_process_opcode(struct emulator_offscreen_buffer *buffer,
                           struct emulator_sound_output_buffer *sound_buffer,
                           struct emulator_keyboard_input *input,
                           struct emulator *emulator)
{
    int_least64_t elapsed_time_ms = platform_get_milliseconds_now() - emulator->timer_start_time_ms;
    float target_frame_rate_ms = (1 / (float)c_target_fps) * 1000;

    if (elapsed_time_ms >= target_frame_rate_ms)
    {                    
        if (emulator->delay_timer > 0)
        {
            --emulator->delay_timer;
        }

        if (emulator->sound_timer > 0)
        {
            --emulator->sound_timer;
        }

        if (emulator->sound_timer < 0) // TODO: sound_timer should never be negative
        {
            OutputDebugStringA("Beep!");
            // TODO: Play sound
        }

        emulator->timer_start_time_ms = platform_get_milliseconds_now();
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
                    
                    uint16_t vx_vy = (uint16_t)(emulator->general_purpose_registers[emulator->x] - emulator->general_purpose_registers[emulator->y]);
                    emulator->general_purpose_registers[emulator->x] = (uint8_t)vx_vy;
                    emulator->general_purpose_registers[0xF] = vx_vy <= UCHAR_MAX ? 1 : 0;                    
                } break;
                case 6:
                {
                    OutputDebugStringA("Vx >>= 1\n");
                    uint8_t lsb = (emulator->general_purpose_registers[emulator->x] & 0x1u);
                    emulator->general_purpose_registers[emulator->x] = (uint8_t)(emulator->general_purpose_registers[emulator->x] >> 1);
                    emulator->general_purpose_registers[0xF] = lsb;
                } break;
                case 7:
                {
                    OutputDebugStringA("Vx = Vy - Vx\n");
                    uint16_t vx_vy = (uint16_t)(emulator->general_purpose_registers[emulator->y] - emulator->general_purpose_registers[emulator->x]);
                    emulator->general_purpose_registers[emulator->x] = (uint8_t)vx_vy;
                    emulator->general_purpose_registers[0xF] = vx_vy <= UCHAR_MAX ? 1 : 0;
                } break;
                case 0xE:
                {
                    OutputDebugStringA("Vx <<= 1\n");
                    uint8_t msb = (uint8_t)((emulator->general_purpose_registers[emulator->x] >> 7) & 0x1u);
                    emulator->general_purpose_registers[emulator->x] = (uint8_t)(emulator->general_purpose_registers[emulator->x] << 1);
                    emulator->general_purpose_registers[0xF] = msb;                   
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
                    uint8_t key = decode_key(emulator->general_purpose_registers[emulator->x]);

                    if (input->buttons[key].is_down)
                    {
                       emulator->pc = (uint16_t)(emulator->pc + 2);
                    }
                } break;
                case 0xA1:
                {
                    OutputDebugString("if (key() != Vx)\n");
                    uint8_t key = decode_key(emulator->general_purpose_registers[emulator->x]);

                    if (!input->buttons[key].is_down)
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
                        emulator->general_purpose_registers[emulator->x] = encode_key((uint8_t)key);
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

uint8_t
encode_key(uint8_t key)
{
    uint8_t value;

    switch (key)
    {
        case 0:
        {
            value = 0x1;
        } break;
        case 1:
        {
            value = 0x2;
        } break;
        case 2:
        {
            value = 0x3;            
        } break;
        case 3:
        {
            value = 0xC;
        } break;
        case 4:
        {
            value = 0x4;
        } break;
        case 5:
        {
            value = 0x5;
        } break;
        case 6:
        {
            value = 0x6;
        } break;
        case 7:
        {
            value = 0xD;
        } break;
        case 8:
        {
            value = 0x7;
        } break;
        case 9:
        {
            value = 0x8;
        } break;
        case 10:
        {
            value = 0x9;
        } break;
        case 11:
        {
            value = 0xE;
        } break;
        case 12:
        {
            value = 0xA;
        } break;
        case 13:
        {
            value = 0x0;
        } break;
        case 14:
        {
            value =0xB;
        } break;
        case 15:
        {
            value = 0xF;
        } break;
        default:
        {
            // TODO: error handling
        } break;
    }

    return value;
}