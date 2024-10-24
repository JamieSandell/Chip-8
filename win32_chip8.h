#include <stdbool.h>
#include <windows.h>

#ifndef WIN32_CHIP8_H
#define WIN32_CHIP8_H

struct win32_offscreen_buffer
{
    // NOTE: Pixels are always 32-bits wide.
    // Memory order:  0x BB GG RR xx
    // Little endian: 0x xx RR GG BB
    int height;
    BITMAPINFO info;
    void *memory;
    int pitch;
    int width;
    int bytes_per_pixel;
};

struct win32_window_dimension
{
    int width;
    int height;
};

static void
win32_display_buffer_in_window(struct win32_offscreen_buffer *buffer, HDC device_context, int window_width, int window_height);

static void
win32_process_keyboard_message(struct emulator_button_state *new_state, bool is_down, bool was_down);

static void
win32_process_pending_messages(struct emulator_keyboard_input *input);

LRESULT CALLBACK
win32_main_window_callback(HWND window, 
                           UINT message,
                           WPARAM w_param,
                           LPARAM l_param);

static void
win32_resize_dib_section(struct win32_offscreen_buffer *buffer, int width, int height);

#endif