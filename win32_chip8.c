#include <stdbool.h>
#include <windows.h>
#include "constants.h"
#include "globals.h"
#include "platform_services.h"
#include "win32_chip8.h"

static struct win32_offscreen_buffer internal_back_buffer;
static bool internal_running;
static int64_t internal_performance_counter_frequency;
static LARGE_INTEGER internal_query_performance_frequency;

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
            QueryPerformanceFrequency(&internal_query_performance_frequency);
            internal_performance_counter_frequency = internal_query_performance_frequency.QuadPart;

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

                struct win32_window_dimension dimension = win32_get_window_dimension(window);
                win32_display_buffer_in_window(&internal_back_buffer, device_context, dimension.width, dimension.height);
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

int_least64_t
platform_get_milliseconds_now(void)
{
    LARGE_INTEGER now;
    QueryPerformanceCounter(&now);
    
    return (1000LL * now.QuadPart) / internal_query_performance_frequency.QuadPart;
}

void
platform_free_file_memory(void *memory)
{
    if (memory)
    {
        VirtualFree(memory, 0, MEM_RELEASE);
    }
}

struct read_file_result
platform_read_entire_file(const char *filename)
{
    struct read_file_result result = {0};
    
    HANDLE file_handle = CreateFileA(filename,
                                     GENERIC_READ,
                                     FILE_SHARE_READ,
                                     NULL,
                                     OPEN_ALWAYS,
                                     0,
                                     0);
    
    if (file_handle != INVALID_HANDLE_VALUE)
    {
        LARGE_INTEGER file_size;
        
        if (GetFileSizeEx(file_handle, &file_size))
        {
            result.contents = VirtualAlloc(0,
                                           file_size.QuadPart,
                                           MEM_RESERVE | MEM_COMMIT,
                                           PAGE_READWRITE);
            
            if (result.contents)
            {
                DWORD bytes_read;
                
                if (ReadFile(file_handle,
                             result.contents,
                             win32_safe_truncate_uint64(file_size.QuadPart),
                             &bytes_read,
                             NULL)
                    && (win32_safe_truncate_uint64(file_size.QuadPart) == bytes_read))
                {
                    result.contents_size = bytes_read;
                }
                else
                {
                    platform_free_file_memory(result.contents);
                    result.contents = NULL;
                    // TODO: Logging, failed to read file.
                }
            }
            else
            {
                // TODO: Logging, memory allocation failed.
            }
        }
        else
        {
            // TODO: Logging, file size evaluation failed.
        }
        
        CloseHandle(file_handle);
    }
    else
    {
        // TODO: Logging, handle creation failed.
    }
    
    return result;
}

static void
win32_display_buffer_in_window(struct win32_offscreen_buffer *buffer, HDC device_context, int window_width, int window_height)
{
    StretchDIBits(device_context,
                  0,
                  0,
                  window_width,
                  window_height,
                  0,
                  0,
                  buffer->width,
                  buffer->height,
                  buffer->memory,
                  &(buffer->info),
                  DIB_RGB_COLORS,
                  SRCCOPY
                  );
}

static struct win32_window_dimension
win32_get_window_dimension(HWND window)
{
    struct win32_window_dimension result;
    RECT client_rect;
    GetClientRect(window, &client_rect);
    result.width = client_rect.right - client_rect.left;
    result.height = client_rect.bottom - client_rect.top;
    return result;
}

LRESULT CALLBACK
win32_main_window_callback(HWND window,
                           UINT message,
                           WPARAM w_param,
                           LPARAM l_param)
{
    LRESULT result = 0;
    
    switch (message)
    {
        case WM_SYSKEYDOWN:
        case WM_SYSKEYUP:
        case WM_KEYDOWN:
        case WM_KEYUP:
        {
            Assert("Keyboard input came in through a non-dispatch message.");
        }break;
        
        case WM_PAINT:
        {
            PAINTSTRUCT paint;
            HDC device_context = BeginPaint(window, &paint);
            struct win32_window_dimension dimension = win32_get_window_dimension(window);
            win32_display_buffer_in_window(&internal_back_buffer, device_context, dimension.width, dimension.height);
            EndPaint(window, &paint);
        } break;
        
        case WM_SIZE:
        {
        } break;
        
        case WM_DESTROY:
        {
            internal_running = false;
        } break;
        
        case WM_CLOSE:
        {
            internal_running = false;
        } break;
        
        case WM_ACTIVATEAPP:
        {
            OutputDebugString("WM_ACTIVATEAPP");
        } break;
        
        default:
        {
            result = DefWindowProc(window, message, w_param, l_param);
        }
        break;
    }
    
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