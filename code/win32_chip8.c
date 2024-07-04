#include <stdbool.h>
#include <stdint.h>
#include <windows.h>

#define internal static
#define local_persist static
#define global_variable static

// unsigned integers
typedef uint8_t u8;
typedef uint16_t u16;
typedef uint32_t u32;
typedef uint64_t u64;
// signed integers
typedef int8_t s8;
typedef int16_t s16;
typedef int32_t s32;
typedef int64_t s64;

global_variable int bitmap_height;
global_variable BITMAPINFO bitmap_info;
global_variable void *bitmap_memory;
global_variable int bitmap_width;
global_variable int bytes_per_pixel;
global_variable bool running;
global_variable int window_width;
global_variable int window_height;

internal void
render_weird_gradient(int x_offset, int y_offset);

LRESULT CALLBACK
win32_main_window_callback(HWND window, 
                           UINT message,
                           WPARAM w_param,
                           LPARAM l_param);

internal void
win32_resize_dib_section(int width, int height);

internal void
win32_update_window(HDC device_context, RECT *client_rect);

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
            running = true;
            int result;
            MSG message;
            
            while ((result = GetMessage(&message, 0, 0, 0)) != 0 && running)
            {
                if (result == -1)
                {
                    // TODO: Logging
                }
                else
                {
                    TranslateMessage(&message);
                    DispatchMessage(&message);
                }
            }
        }
        else
        {
            // TODO: Logging
        }
    }
    else
    {
        //TODO: Logging
    }
    
    return (0);
}

internal void
render_weird_gradient(int x_offset, int y_offset)
{
    int pitch = bitmap_width * bytes_per_pixel;
    u8 *row = (u8 *)bitmap_memory;
    
    for (int y = 0; y < bitmap_height; ++y)
    {
        u8 *pixel = (u8 *)row;
        
        for (int x = 0; x < bitmap_width; ++x)
        {
            *pixel = (u8)(x + x_offset);
            //*pixel = 0;
            ++pixel;
            
            *pixel = (u8)(y + y_offset);
            //*pixel = 0;
            ++pixel;
            
            *pixel = (u8)row;
            //*pixel = 0;
            ++pixel;
            
            *pixel = 0;
            ++pixel;
        }
        
        row += pitch;
    }
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
        case WM_PAINT:
        {
            PAINTSTRUCT paint;
            HDC device_context = BeginPaint(window, &paint);
            RECT client_rect;
            GetClientRect(window, &client_rect);
            win32_update_window(device_context, &client_rect);
            EndPaint(window, &paint);
        } break;
        
        case WM_SIZE:
        {
            RECT client_rect;
            GetClientRect(window, &client_rect);
            int width = client_rect.right - client_rect.left;
            int height = client_rect.bottom - client_rect.top;
            win32_resize_dib_section(width, height);
        } break;
        
        case WM_DESTROY:
        {
            running = false;
        } break;
        
        case WM_CLOSE:
        {
            running = false;
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

internal void
win32_resize_dib_section(int width, int height)
{
    bitmap_width = width;
    bitmap_height = height;
    
    bitmap_info.bmiHeader.biSize = sizeof(bitmap_info.bmiHeader);
    bitmap_info.bmiHeader.biWidth = bitmap_width;
    bitmap_info.bmiHeader.biHeight = -bitmap_height; // negative for top-down stride
    bitmap_info.bmiHeader.biPlanes = 1;
    bitmap_info.bmiHeader.biBitCount = 32; // 8 bits per colour, r, g, b = 24, but 32 for alignment.
    bitmap_info.bmiHeader.biCompression = BI_RGB;
    
    if (bitmap_memory)
    {
        VirtualFree(bitmap_memory, 0, MEM_RELEASE);
    }
    
    bytes_per_pixel = 4;
    int bitmap_memory_size = bytes_per_pixel * (bitmap_width * bitmap_height);
    bitmap_memory = VirtualAlloc(0, bitmap_memory_size, MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE);
    render_weird_gradient(128, 0);
    
}

internal void
win32_update_window(HDC device_context, RECT *client_rect)
{
    window_width = client_rect->right - client_rect->left;
    window_height = client_rect->bottom - client_rect->top;
    StretchDIBits(device_context,
                  0,
                  0,
                  window_width,
                  window_height,
                  0,
                  0,
                  bitmap_width,
                  bitmap_height,
                  bitmap_memory,
                  &bitmap_info,
                  DIB_RGB_COLORS,
                  SRCCOPY
                  );
}