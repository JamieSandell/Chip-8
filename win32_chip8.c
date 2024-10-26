#include <stdbool.h>
#include <windows.h>
#include "constants.h"
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
                
                emulator_update_and_render(&bitmap_buffer, NULL, &keyboard_input, &emulator);
                
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

static void
win32_process_keyboard_message(struct emulator_button_state *new_state, bool is_down, bool was_down)
{
    new_state->is_down = is_down;
    new_state->was_down = was_down;
}

static void
win32_process_pending_messages(struct emulator_keyboard_input *input)
{
    MSG message;
    while(PeekMessageA(&message, 0, 0, 0, PM_REMOVE))
    {
        switch (message.message)
        {
            case WM_QUIT:
            {
                internal_running = false;
            } break;
            
            case WM_SYSKEYDOWN:
            case WM_SYSKEYUP:
            case WM_KEYDOWN:
            case WM_KEYUP:
            {
                bool is_down = ((message.lParam & (1 << 31)) == 0); // NOTE: The transition state. The value is always 0 for a WM_KEYDOWN message.
                bool was_down = ((message.lParam & (1 << 30)) != 0); // NOTE: The previous key state. The value is 1 if the key is down before the message is sent, or it is zero if the key is up.
                uint32_t vk_code = (uint32_t)message.wParam;
                
                if (is_down)
                {
                    if (vk_code == '1')
                    {
                        win32_process_keyboard_message(&input->numeric_1, is_down, was_down);
                    }
                    else if (vk_code == '2')
                    {
                        win32_process_keyboard_message(&input->numeric_2, is_down, was_down);
                    }
                    else if (vk_code == '3')
                    {
                        win32_process_keyboard_message(&input->numeric_3, is_down, was_down);
                    }
                    else if (vk_code == '4')
                    {
                        win32_process_keyboard_message(&input->numeric_4, is_down, was_down);
                    }
                    else if (vk_code == 'Q')
                    {
                        win32_process_keyboard_message(&input->Q, is_down, was_down);
                    }
                    else if (vk_code == 'W')
                    {
                        win32_process_keyboard_message(&input->W, is_down, was_down);
                    }
                    else if (vk_code == 'E')
                    {
                        win32_process_keyboard_message(&input->E, is_down, was_down);
                    }
                    else if (vk_code == 'R')
                    {
                        win32_process_keyboard_message(&input->R, is_down, was_down);
                    }
                    else if (vk_code == 'A')
                    {
                        win32_process_keyboard_message(&input->A, is_down, was_down);
                    }
                    else if (vk_code == 'S')
                    {
                        win32_process_keyboard_message(&input->S, is_down, was_down);
                    }
                    else if (vk_code == 'D')
                    {
                        win32_process_keyboard_message(&input->D, is_down, was_down);
                    }
                    else if (vk_code == 'F')
                    {
                        win32_process_keyboard_message(&input->F, is_down, was_down);
                    }
                    else if (vk_code == 'Z')
                    {
                        win32_process_keyboard_message(&input->Z, is_down, was_down);
                    }
                    else if (vk_code == 'X')
                    {
                        win32_process_keyboard_message(&input->X, is_down, was_down);
                    }
                    else if (vk_code == 'C')
                    {
                        win32_process_keyboard_message(&input->C, is_down, was_down);
                    }
                    else if (vk_code == 'V')
                    {
                        win32_process_keyboard_message(&input->V, is_down, was_down);
                    }
                }
                
                if (is_down != was_down)
                {
                    bool alt_key_was_down = ((message.lParam & (1 << 29)) != 0);
                    
                    if ((vk_code == VK_F4) && alt_key_was_down)
                    {
                        internal_running = false;
                    }
                } break;
                
                default:
                {
                    TranslateMessage(&message);
                    DispatchMessage(&message);
                } break;
            }
        }
    }
}

static void
win32_resize_dib_section(struct win32_offscreen_buffer *buffer, int width, int height)
{
    buffer->width = width;
    buffer->height = height;
    
    buffer->info.bmiHeader.biSize = sizeof(buffer->info.bmiHeader);
    buffer->info.bmiHeader.biWidth = buffer->width;
    buffer->info.bmiHeader.biHeight = -buffer->height; // negative for top-down stride
    buffer->info.bmiHeader.biPlanes = 1;
    buffer->info.bmiHeader.biBitCount = 32; // 8 bits per colour, r, g, b = 24, but 32 for alignment.
    buffer->info.bmiHeader.biCompression = BI_RGB;
    
    // Note: Remember to VirtualFree the memory if we ever call this function
    // more than once on the same buffer.
    
    buffer->bytes_per_pixel = 4;
    buffer->pitch = buffer->width * buffer->bytes_per_pixel;
    int bitmap_memory_size = buffer->bytes_per_pixel * (buffer->width * buffer->height);
    buffer->memory = VirtualAlloc(0, bitmap_memory_size, MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE);
}