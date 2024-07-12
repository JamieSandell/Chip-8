#include <dsound.h>
#include <math.h>
#include <stdint.h>
#include <xaudio2.h>
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
enum {false, true};

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
    int tone_volume;
};

struct win32_sound_buffer
{
    DWORD play_cursor;
    DWORD write_cursor;
    u32 running_sample_index;
    int secondary_buffer_size;
    int bytes_per_sample;
    int half_wave_period;
    int tone_hz;
    int wave_period;
    int samples_per_second;
    int tone_volume;
    b32 is_playing;
    IDirectSoundBuffer *secondary_buffer;
};

struct win32_window_dimension
{
    int width;
    int height;
};


global_variable b32 global_running;
global_variable struct win32_offscreen_buffer global_back_buffer;
global_variable struct win32_sound_buffer global_sound_buffer;

internal void
render_weird_gradient(struct win32_offscreen_buffer *buffer, int x_offset, int y_offset);

internal void
win32_display_buffer_in_window(struct win32_offscreen_buffer *buffer, HDC device_context, int window_width, int window_height);

internal struct win32_window_dimension
win32_get_window_dimension(HWND window);

internal void
win32_init_d_sound(HWND window, s32 samples_per_second, s32 buffer_size);

internal void
win32_init_x_audio(void);

internal void
win32_update_d_sound(void);

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
            global_running = true;
            int x_offset = 0;
            int y_offset = 0;
            
            global_sound_buffer.bytes_per_sample = sizeof(s16) * 2;
            global_sound_buffer.tone_hz = 1000;
            global_sound_buffer.samples_per_second = 48000;
            global_sound_buffer.tone_volume = 3000;
            global_sound_buffer.running_sample_index = 0;
            global_sound_buffer.wave_period = global_sound_buffer.samples_per_second / global_sound_buffer.tone_hz;
            global_sound_buffer.half_wave_period = global_sound_buffer.wave_period / 2;
            global_sound_buffer.secondary_buffer_size = 2 * global_sound_buffer.samples_per_second * global_sound_buffer.bytes_per_sample;
            global_sound_buffer.is_playing = false;
            win32_init_d_sound(window, global_sound_buffer.samples_per_second, global_sound_buffer.secondary_buffer_size);
            
            //win32_init_x_audio();
            
            while (global_running)
            {
                MSG message;
                while(PeekMessageA(&message, 0, 0, 0, PM_REMOVE))
                {
                    TranslateMessage(&message);
                    DispatchMessage(&message);
                }
                
                // NOTE: DirectSound test
                DWORD play_cursor;
                DWORD write_cursor;
                
                win32_update_d_sound();
                
                render_weird_gradient(&global_back_buffer, x_offset, y_offset);
                ++x_offset;
                
                struct win32_window_dimension dimension = win32_get_window_dimension(window);
                win32_display_buffer_in_window(&global_back_buffer, device_context, dimension.width, dimension.height);
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
            WAVEFORMATEX wave_format = {0};
            wave_format.wFormatTag = WAVE_FORMAT_PCM;
            wave_format.nChannels = 2;
            wave_format.nSamplesPerSec = samples_per_second;
            wave_format.wBitsPerSample = 16;
            wave_format.nBlockAlign = (wave_format.nChannels * wave_format.wBitsPerSample) / 8;
            wave_format.nAvgBytesPerSec = wave_format.nSamplesPerSec * wave_format.nBlockAlign;
            
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
                
                if (direct_sound->lpVtbl->CreateSoundBuffer(direct_sound, &buffer_description, &primary_buffer, NULL) == DS_OK)
                {
                    if (primary_buffer->lpVtbl->SetFormat(primary_buffer, &wave_format) == DS_OK)
                    {
                        OutputDebugString("Primary buffer was set.\n");
                    }
                    else
                    {
                        // TODO: Diagnostics
                    }
                }
                else
                {
                    // TODO: Diagnostics
                }
                
                DSBUFFERDESC secondary_buffer_description = {0};
                secondary_buffer_description.dwSize = sizeof(secondary_buffer_description);
                secondary_buffer_description.dwBufferBytes = buffer_size;
                secondary_buffer_description.lpwfxFormat = &wave_format;
                
                if (direct_sound->lpVtbl->CreateSoundBuffer(direct_sound, &secondary_buffer_description, &global_sound_buffer.secondary_buffer, NULL) == DS_OK)
                {
                    OutputDebugString("Secondary buffer created.\n");
                }
                else
                {
                    // TODO: Diagnostics
                }
            }
            else
            {
                // TODO: Diagnostics
            }
        }
        else
        {
            // TODO: Diagnostics
        }
    }
    else
    {
        // TODO: Diagnostics
    }
}

internal void
win32_init_x_audio(void)
{
    IXAudio2 *x_audio = NULL;
    if (XAudio2Create(&x_audio, 0, XAUDIO2_DEFAULT_PROCESSOR) == S_OK)
    {
        OutputDebugString("Instance of XAudio2 engine created.\n");
        IXAudio2MasteringVoice *master_voice = NULL;
        
        if (x_audio->lpVtbl->CreateMasteringVoice(x_audio,
                                                  &master_voice,
                                                  XAUDIO2_DEFAULT_CHANNELS,
                                                  XAUDIO2_DEFAULT_SAMPLERATE,
                                                  0,
                                                  NULL,
                                                  NULL,
                                                  AudioCategory_GameEffects) == S_OK)
        {
            OutputDebugString("Mastering voice reated.\n");
        }
        else
        {
            // TODO: Diagnostics
        }
    }
    else
    {
        // TODO: Diagnostics
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
            b32 is_down = ((l_param & (1 << 31)) == 0);
            b32 was_down = ((l_param & (1 << 30)) != 0);
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

internal void
win32_update_d_sound(void)
{
    if (global_sound_buffer.secondary_buffer->lpVtbl->GetCurrentPosition(global_sound_buffer.secondary_buffer, &global_sound_buffer.play_cursor, &global_sound_buffer.write_cursor) == DS_OK)
    {
        DWORD byte_to_lock = global_sound_buffer.running_sample_index * global_sound_buffer.bytes_per_sample % global_sound_buffer.secondary_buffer_size;
        DWORD bytes_to_write;
        
        if (byte_to_lock == global_sound_buffer.play_cursor)
        {
            // Play cursor is at the same spot
            bytes_to_write = global_sound_buffer.secondary_buffer_size;
        }
        else if (byte_to_lock > global_sound_buffer.play_cursor)
        {
            // Play cursor is behind
            bytes_to_write = global_sound_buffer.secondary_buffer_size - byte_to_lock; // region 1
            bytes_to_write += global_sound_buffer.play_cursor; // region 2
        }
        else
        {
            // Play cursor is in front
            bytes_to_write = global_sound_buffer.play_cursor - byte_to_lock; // region 1
        }
        
        VOID *region1;
        DWORD region1_size;
        VOID *region2;
        DWORD region2_size;
        
        if (global_sound_buffer.secondary_buffer->lpVtbl->Lock(global_sound_buffer.secondary_buffer,
                                                               byte_to_lock, bytes_to_write,
                                                               &region1, &region1_size,
                                                               &region2, &region2_size,
                                                               0) == DS_OK)
        {
            // TODO: Assert that region1_size and region2_size are valid
            
            s16 *sample_out = (s16 *)region1;
            DWORD region1_sample_count = region1_size / global_sound_buffer.bytes_per_sample;
            
            for (DWORD sample_index = 0; sample_index < region1_sample_count; ++sample_index)
            {
                s16 sample_value = ((global_sound_buffer.running_sample_index++ / global_sound_buffer.half_wave_period) % 2) ? global_sound_buffer.tone_volume : -global_sound_buffer.tone_volume;
                *sample_out++ = sample_value;
                *sample_out++ = sample_value;
            }
            
            sample_out = (s16 *)region2;
            DWORD region2_sample_count = region2_size / global_sound_buffer.bytes_per_sample;
            
            for (DWORD sample_index = 0; sample_index < region2_sample_count; ++sample_index)
            {
                s16 sample_value = ((global_sound_buffer.running_sample_index++ / global_sound_buffer.half_wave_period) % 2) ? global_sound_buffer.tone_volume : -global_sound_buffer.tone_volume;
                *sample_out++ = sample_value;
                *sample_out++ = sample_value;
            }
            
            global_sound_buffer.secondary_buffer->lpVtbl->Unlock(global_sound_buffer.secondary_buffer, region1, region1_size, region2, region2_size);
            
            if (!global_sound_buffer.is_playing)
            {
                global_sound_buffer.secondary_buffer->lpVtbl->Play(global_sound_buffer.secondary_buffer, 0, 0, DSBPLAY_LOOPING);
                global_sound_buffer.is_playing = true;
            }
        }
    }
}