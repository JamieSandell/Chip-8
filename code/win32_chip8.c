#include <dsound.h>
#include <stdbool.h>
#include <stdint.h>
#include <windows.h>

#define internal static
#define local_persist static
#define global_variable static

typedef uint8_t u8;
typedef uint16_t u16;
typedef uint32_t u32;
typedef uint64_t u64;

typedef int8_t s8;
typedef int16_t s16;
typedef int32_t s32;
typedef int64_t s64;

typedef s32 b32;

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


global_variable bool global_running;
global_variable struct win32_offscreen_buffer global_back_buffer;

internal void
render_weird_gradient(struct win32_offscreen_buffer *buffer, int x_offset, int y_offset);

internal void
win32_display_buffer_in_window(struct win32_offscreen_buffer *buffer, HDC device_context, int window_width, int window_height);

internal struct win32_window_dimension
win32_get_window_dimension(HWND window, s32 samples_per_second, s32 buffer_size);

internal void
win32_init_d_sound(HWND window);

// NOTE: DirectSoundCreate
#define DIRECT_SOUND_CREATE(name) HRESULT WINAPI name(LPCGUID guid_device, LPDIRECTSOUND *ds, LPUNKNOWN unk_outer)
typedef DIRECT_SOUND_CREATE(direct_sound_create);


LRESULT CALLBACK
win32_main_window_callback(HWND window, 
                           UINT message,
                           WPARAM w_param,
                           LPARAM l_param);


internal void
win32_resize_dib_section(struct win32_offscreen_buffer *buffer, int width, int height);

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
            HDC device_context = GetDC(window); // NOTE: Since we specified SC_OWNDC we can use it forever, no need to get and release every loop iteration.
            win32_resize_dib_section(&global_back_buffer, 1280, 720);
            win32_init_d_sound(window);
            global_running = true;
            int x_offset = 0;
            int y_offset = 0;
            
            while (global_running)
            {
                
                MSG message;
                while(PeekMessageA(&message, 0, 0, 0, PM_REMOVE))
                {
                    TranslateMessage(&message);
                    DispatchMessage(&message);
                }
                
                render_weird_gradient(&global_back_buffer, x_offset, y_offset);
                ++x_offset;
                
                struct win32_window_dimension dimension = win32_get_window_dimension(window);
                win32_display_buffer_in_window(&global_back_buffer, device_context, dimension.width, dimension.height);
            }
        }
        else
        {
            //TODO: Logging
        }
        
        return 0;
    }
    
    return -1;
}

internal void
render_weird_gradient(struct win32_offscreen_buffer *buffer, int x_offset, int y_offset)
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

internal void
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

internal struct win32_window_dimension
win32_get_window_dimension(HWND window)
{
    struct win32_window_dimension result;
    RECT client_rect;
    GetClientRect(window, &client_rect);
    result.width = client_rect.right - client_rect.left;
    result.height = client_rect.bottom - client_rect.top;
    return result;
}

internal void
win32_init_d_sound(HWND window, s32 samples_per_second, s32 buffer_size)
{
    HMODULE d_sound_library = LoadLibrary("dsound.dll");
    
    if (d_sound_library)
    {
        direct_sound_create *DirectSoundCreate = (direct_sound_create *)GetProcAddress(d_sound_library, "DirectSoundCreate");
        IDirectSound *direct_sound;
        if (DirectSoundCreate && SUCCEEDED(DirectSoundCreate(NULL, &direct_sound, NULL)))
        {
            if (direct_sound->lpVtbl->SetCooperativeLevel(direct_sound, window, DSSCL_PRIORITY) == DS_OK)
            {
                DSBUFFERDESC buffer_description = {0};
                buffer_description.dwSize = sizeof(buffer_description);
                buffer_description.dwFlags = DSBCAPS_PRIMARYBUFFER;
                buffer_description.dwBufferBytes = 0;
                buffer_description.dwReserved = 0;
                buffer_description.lpwfxFormat = NULL;
                buffer_description.guid3DAlgorithm = GUID_NULL;
                IDirectSoundBuffer *primary_buffer;
                
                if (direct_sound->lpVtbl->CreateSoundBuffer(direct_sound, &buffer_description, &primary_buffer, NULL))
                {
                }
            }
        }
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
        case WM_SYSKEYDOWN:
        case WM_SYSKEYUP:
        case WM_KEYDOWN:
        case WM_KEYUP:
        {
            bool is_down = ((l_param & (1 << 31)) == 0);
            bool was_down = ((l_param & (1 << 30)) != 0);
            u32 vk_code = w_param;
            
            if (is_down != was_down)
            {
                b32 alt_key_was_down = ((l_param & (1 << 29)) != 0);
                
                if ((vk_code == VK_F4) && alt_key_was_down)
                {
                    global_running = false;
                }
            } break;
            
            case WM_PAINT:
            {
                PAINTSTRUCT paint;
                HDC device_context = BeginPaint(window, &paint);
                struct win32_window_dimension dimension = win32_get_window_dimension(window);
                win32_display_buffer_in_window(&global_back_buffer, device_context, dimension.width, dimension.height);
                EndPaint(window, &paint);
            } break;
            
            case WM_SIZE:
            {
            } break;
            
            case WM_DESTROY:
            {
                global_running = false;
            } break;
            
            case WM_CLOSE:
            {
                global_running = false;
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
    }
    
    return result;
}

internal void
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