#include <dsound.h>
#include <math.h>
#include <stdint.h>
#include <xaudio2.h>
#include <windows.h>

#define internal static
#define local_persist static
#define global_variable static
#define Pi32 3.14159265359f

typedef uint8_t u8;
typedef uint16_t u16;
typedef uint32_t u32;
typedef uint64_t u64;

typedef int8_t s8;
typedef int16_t s16;
typedef int32_t s32;
typedef int64_t s64;

typedef float f32;
typedef double f64;

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

struct win32_sound_output
{
    u32 running_sample_index;
    int secondary_buffer_size;
    int bytes_per_sample;
    int tone_hz;
    int wave_period;
    int samples_per_second;
    int tone_volume;
    IDirectSoundBuffer *secondary_buffer;
    int latency_sample_count;
    f32 t_sine;
};

struct win32_window_dimension
{
    int width;
    int height;
};


global_variable b32 global_running;
global_variable struct win32_offscreen_buffer global_back_buffer;

internal void
render_weird_gradient(struct win32_offscreen_buffer *buffer, int x_offset, int y_offset);

internal void
win32_display_buffer_in_window(struct win32_offscreen_buffer *buffer, HDC device_context, int window_width, int window_height);

internal struct win32_window_dimension
win32_get_window_dimension(HWND window);

internal void
win32_fill_sound_buffer(struct win32_sound_output *sound_output, DWORD byte_to_lock, DWORD bytes_to_write);

internal void
win32_init_d_sound(HWND window, struct win32_sound_output *sound_output);

internal void
win32_init_x_audio(void);

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
    LARGE_INTEGER perf_count_frequency_result;
    QueryPerformanceFrequency(&perf_count_frequency_result);
    s64 perf_count_frequency = perf_count_frequency_result.QuadPart;
    
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
            LARGE_INTEGER last_counter;
            QueryPerformanceCounter(&last_counter);
            int x_offset = 0;
            int y_offset = 0;
            
            struct win32_sound_output sound_output = {};
            sound_output.bytes_per_sample = sizeof(s16) * 2;
            sound_output.tone_hz = 256;
            sound_output.samples_per_second = 48000;
            sound_output.tone_volume = 3000;
            sound_output.running_sample_index = 0;
            sound_output.wave_period = sound_output.samples_per_second / sound_output.tone_hz;
            sound_output.secondary_buffer_size = sound_output.samples_per_second * sound_output.bytes_per_sample;
            sound_output.latency_sample_count = sound_output.samples_per_second / 15;
            win32_init_d_sound(window, &sound_output);
            win32_fill_sound_buffer(&sound_output, 0, sound_output.latency_sample_count * sound_output.bytes_per_sample);
            sound_output.secondary_buffer->lpVtbl->Play(sound_output.secondary_buffer, 0, 0, DSBPLAY_LOOPING);
            
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
                
                if (sound_output.secondary_buffer->lpVtbl->GetCurrentPosition(sound_output.secondary_buffer, &play_cursor, &write_cursor) == DS_OK)
                {
                    DWORD byte_to_lock = ((sound_output.running_sample_index * sound_output.bytes_per_sample) % sound_output.secondary_buffer_size);
                    DWORD bytes_to_write;
                    DWORD target_cursor = ((play_cursor + (sound_output.latency_sample_count * sound_output.bytes_per_sample)) % sound_output.secondary_buffer_size);
                    
                    if (byte_to_lock > target_cursor)
                    {
                        // Play cursor is behind
                        bytes_to_write = sound_output.secondary_buffer_size - byte_to_lock; // region 1
                        bytes_to_write += target_cursor; // region 2
                    }
                    else
                    {
                        // Play cursor is in front
                        bytes_to_write = target_cursor - byte_to_lock; // region 1
                    }
                    
                    win32_fill_sound_buffer(&sound_output, byte_to_lock, bytes_to_write);
                }
                
                render_weird_gradient(&global_back_buffer, x_offset, y_offset);
                ++x_offset;
                
                struct win32_window_dimension dimension = win32_get_window_dimension(window);
                win32_display_buffer_in_window(&global_back_buffer, device_context, dimension.width, dimension.height);
                
                LARGE_INTEGER end_counter;
                QueryPerformanceCounter(&end_counter);
                s64 counter_elapsed = end_counter.QuadPart - last_counter.QuadPart;
                s32 ms_per_frame = (s32)((1000 * counter_elapsed) / perf_count_frequency);
                char buffer[256];
                wsprintfA(buffer, "ms/frame: %dms\n", ms_per_frame);
                OutputDebugStringA(buffer);
                last_counter = end_counter;
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
win32_init_d_sound(HWND window, struct win32_sound_output *sound_output)
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
            wave_format.nSamplesPerSec = sound_output->samples_per_second;
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
                secondary_buffer_description.dwBufferBytes = sound_output->secondary_buffer_size;
                secondary_buffer_description.lpwfxFormat = &wave_format;
                
                if (direct_sound->lpVtbl->CreateSoundBuffer(direct_sound, &secondary_buffer_description, &sound_output->secondary_buffer, NULL) == DS_OK)
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
win32_fill_sound_buffer(struct win32_sound_output *sound_output, DWORD byte_to_lock, DWORD bytes_to_write)
{
    VOID *region1;
    DWORD region1_size;
    VOID *region2;
    DWORD region2_size;
    
    if (sound_output->secondary_buffer->lpVtbl->Lock(sound_output->secondary_buffer,
                                                     byte_to_lock, bytes_to_write,
                                                     &region1, &region1_size,
                                                     &region2, &region2_size,
                                                     0) == DS_OK)
    {
        // TODO: Assert that region1_size and region2_size are valid
        
        s16 *sample_out = (s16 *)region1;
        DWORD region1_sample_count = region1_size / sound_output->bytes_per_sample;
        
        for (DWORD sample_index = 0; sample_index < region1_sample_count; ++sample_index)
        {
            f32 sine_value = sinf(sound_output->t_sine);
            s16 sample_value = (s16)(sine_value * sound_output->tone_volume);
            *sample_out++ = sample_value;
            *sample_out++ = sample_value;
            sound_output->t_sine += (2.0f * Pi32 * 1.0f) / (f32)sound_output->wave_period;
            sound_output->running_sample_index++;
        }
        
        sample_out = (s16 *)region2;
        DWORD region2_sample_count = region2_size / sound_output->bytes_per_sample;
        
        for (DWORD sample_index = 0; sample_index < region2_sample_count; ++sample_index)
        {
            f32 sine_value = sinf(sound_output->t_sine);
            s16 sample_value = (s16)(sine_value * sound_output->tone_volume);
            *sample_out++ = sample_value;
            *sample_out++ = sample_value;
            sound_output->t_sine += (2.0f * Pi32 * 1.0f) / (f32)sound_output->wave_period;
            sound_output->running_sample_index++;
        }
        
        sound_output->secondary_buffer->lpVtbl->Unlock(sound_output->secondary_buffer, region1, region1_size, region2, region2_size);
    }
}