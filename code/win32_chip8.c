#include <stdbool.h>
#include <windows.h>

#define internal static
#define local_persist static
#define global_variable static

global_variable BITMAPINFO bitmap_info;
global_variable void *bitmap_memory;
global_variable bool running;

LRESULT CALLBACK
win32_main_window_callback(HWND window, 
                           UINT message,
                           WPARAM w_param,
                           LPARAM l_param);

internal void
win32_resize_dib_section(int width, int height);

internal void
win32_update_window(HDC device_context, int x, int y, int width, int height);

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
            int x = paint.rcPaint.left;
            int y = paint.rcPaint.top;
            int width = paint.rcPaint.right - x;
            int height = paint.rcPaint.bottom - y;
            win32_update_window(device_context, x, y, width, height);
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
    bitmap_info.bmiHeader.biSize = sizeof(bitmap_info.bmiHeader);
    bitmap_info.bmiHeader.biWidth = width;
    bitmap_info.bmiHeader.biHeight = height;
    bitmap_info.bmiHeader.biPlanes = 1;
    bitmap_info.bmiHeader.biBitCount = 32; // 8 bits per colour, r, g, b = 24, but 32 for alignment.
    bitmap_info.bmiHeader.biCompression = BI_RGB;
    int bytes_per_pixel = 4;
    int bitmap_memory_size = bytes_per_pixel * (width * height);
    bitmap_memory = VirtualAlloc(0, bitmap_memory_size, MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE);
}

internal void
win32_update_window(HDC device_context, int x, int y, int width, int height)
{
    StretchDIBits(device_context,
                  x,
                  y,
                  width,
                  height,
                  x,
                  y,
                  width,
                  height,
                  bitmap_memory,
                  &bitmap_info,
                  DIB_RGB_COLORS,
                  SRCCOPY
                  );
}