#include <stdbool.h>
#include <windows.h>
#include "win32_chip8.h"

static struct win32_offscreen_buffer internal_back_buffer;
static bool internal_running;

int CALLBACK
WinMain (HINSTANCE instance,
         HINSTANCE prev_instance,
         LPSTR     command_line,
         int       show_command)
{    
    WNDCLASS window_class = {0};
    window_class.style = CS_OWNDC | CS_HREDRAW | CS_VREDRAW;
    window_class.lpfnWndProc = win32_main_window_callback;
    // WindowClass.hIcon;
    window_class.lpszClassName = "Chip8WindowClass";
    
    if(RegisterClass(&window_class))
    {
        HWND window = CreateWindowEx(
                                     0,
                                     window_class.lpszClassName,
                                     "Chip-8",
                                     WS_OVERLAPPEDWINDOW | WS_VISIBLE,
                                     CW_USEDEFAULT,
                                     CW_USEDEFAULT,
                                     CW_USEDEFAULT,
                                     CW_USEDEFAULT,
                                     0,
                                     0,
                                     instance,
                                     0
                                     );
        
        if (window)
        {
            HDC device_context = GetDC(window);
            win32_resize_dib_section(&internal_back_buffer, C_DISPLAY_WIDTH, C_DISPLAY_HEIGHT);
            internal_running = true;

            struct emulator_keyboard_input keyboard_input = {0};
            const struct emulator_keyboard_input empty_keyboard_input = {0};            
            struct emulator emulator = {0};

            emulator_init(&emulator);
            
            while (internal_running)
            {
                win32_process_pending_messages(&keyboard_input);

                struct emulator_offscreen_buffer bitmap_buffer = {0};

                bitmap_buffer.memory = internal_back_buffer.memory;
                bitmap_buffer.width = internal_back_buffer.width;
                bitmap_buffer.height = internal_back_buffer.height;
                bitmap_buffer.pitch = internal_back_buffer.pitch;
                
                emulator_update_and_render(&bitmap_buffer, &sound_buffer, &keyboard_input, &emulator);
                
                keyboard_input = empty_keyboard_input;
            }
        }
        else
        {
            //TODO: Diagnostics
        }
        
        return 0;
    }
    
    return -1;
}

LRESULT CALLBACK
win32_main_window_callback(HWND window,
                           UINT message,
                           WPARAM w_param,
                           LPARAM l_param)
{
    LRESULT result = 0;

    return result;
}