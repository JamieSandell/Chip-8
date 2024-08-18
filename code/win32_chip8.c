#include <dsound.h>
#include <math.h>
#include <Profileapi.h>
#include <stdint.h>
#include <stdio.h>
#include <xaudio2.h>
#include <windows.h>
#include "win32_chip8.h"

#include "constants.c"
#include "globals.c"
#include "stack.c"
#include "chip8.c"

global_variable b32 global_running;
global_variable struct win32_offscreen_buffer global_back_buffer;
global_variable LARGE_INTEGER global_query_performance_frequency;

int CALLBACK
WinMain (HINSTANCE instance,
         HINSTANCE prev_instance,
         LPSTR     command_line,
         int       show_command)
{
    QueryPerformanceFrequency(&global_query_performance_frequency);
    LARGE_INTEGER perf_count_frequency_result; // TODO: remove
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
            win32_resize_dib_section(&global_back_buffer, C_DISPLAY_WIDTH, C_DISPLAY_HEIGHT);
            global_running = true;
            LARGE_INTEGER last_counter;
            QueryPerformanceCounter(&last_counter);
            u64 last_cycle_count = __rdtsc();
            
            struct win32_sound_output sound_output = {};
            sound_output.bytes_per_sample = sizeof(s16) * 2;
            sound_output.samples_per_second = 48000;
            sound_output.running_sample_index = 0;
            sound_output.secondary_buffer_size = sound_output.samples_per_second * sound_output.bytes_per_sample;
            sound_output.latency_sample_count = sound_output.samples_per_second / 15;
            win32_init_d_sound(window, &sound_output);
            win32_clear_sound_buffer(&sound_output);
            sound_output.secondary_buffer->lpVtbl->Play(sound_output.secondary_buffer, 0, 0, DSBPLAY_LOOPING);
            
            //win32_init_x_audio();
            
            struct emulator_keyboard_input inputs[2] = {0};
            
            struct emulator emulator = {0};
            emulator_init(&emulator);
            
            while (global_running)
            {
                struct emulator_keyboard_input *old_input = &inputs[0];
                struct emulator_keyboard_input *new_input = &inputs[1];
                struct emulator_keyboard_input *temp_input = NULL;
                
                win32_process_pending_messages(new_input);
                
                // NOTE: DirectSound test
                DWORD byte_to_lock;
                DWORD bytes_to_write;
                DWORD target_cursor;
                DWORD play_cursor;
                DWORD write_cursor;
                b32 is_sound_valid = false;
                
                if (sound_output.secondary_buffer->lpVtbl->GetCurrentPosition(sound_output.secondary_buffer, &play_cursor, &write_cursor) == DS_OK)
                {
                    
                    byte_to_lock = ((sound_output.running_sample_index * sound_output.bytes_per_sample) % sound_output.secondary_buffer_size);
                    bytes_to_write;
                    target_cursor = ((play_cursor + (sound_output.latency_sample_count * sound_output.bytes_per_sample)) % sound_output.secondary_buffer_size);
                    
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
                    
                    is_sound_valid = true;
                }
                
                s16 *samples = (s16 *)VirtualAlloc(0, sound_output.secondary_buffer_size,
                                                   MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE);
                struct emulator_sound_output_buffer sound_buffer = {0};
                sound_buffer.samples_per_second = sound_output.samples_per_second;
                sound_buffer.sample_count = bytes_to_write / sound_output.bytes_per_sample;
                sound_buffer.samples = samples;
                
                struct emulator_offscreen_buffer bitmap_buffer = {0};
                bitmap_buffer.memory = global_back_buffer.memory;
                bitmap_buffer.width = global_back_buffer.width;
                bitmap_buffer.height = global_back_buffer.height;
                bitmap_buffer.pitch = global_back_buffer.pitch;
                
                emulator_update_and_render(&bitmap_buffer, &sound_buffer, new_input, &emulator);
                
                temp_input = new_input;
                new_input = old_input;
                old_input = temp_input;
                
                if (is_sound_valid)
                {
                    
                    win32_fill_sound_buffer(&sound_output, byte_to_lock, bytes_to_write, &sound_buffer);
                }
                
                struct win32_window_dimension dimension = win32_get_window_dimension(window);
                win32_display_buffer_in_window(&global_back_buffer, device_context, dimension.width, dimension.height);
                
                LARGE_INTEGER end_counter;
                QueryPerformanceCounter(&end_counter);
                u64 end_cycle_count = __rdtsc();
                s64 counter_elapsed = end_counter.QuadPart - last_counter.QuadPart;
                s64 cycles_elapsed = end_cycle_count - last_cycle_count;
                f32 ms_per_frame = 1000.0f * (f32)counter_elapsed / (f32)perf_count_frequency;
                f32 fps = (f32)(perf_count_frequency / (f32) counter_elapsed);
                f32 mega_cycles_per_frame = (f32)cycles_elapsed / (1000.0f * 1000.0f);
#if 0
                char buffer[256];
                sprintf(buffer, "%.02f MS Per Frame, %.02ff FPS, %.02f Megacycles Per Frame\n", ms_per_frame, fps, mega_cycles_per_frame);
                OutputDebugStringA(buffer);
#endif
                last_counter = end_counter;
                last_cycle_count = end_cycle_count;
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

internal int_least64_t
platform_get_milliseconds_now(void)
{
    LARGE_INTEGER now;
    QueryPerformanceCounter(&now);
    
    return (1000LL * now.QuadPart) / global_query_performance_frequency.QuadPart;
}

internal void
platform_free_file_memory(void *memory)
{
    if (memory)
    {
        VirtualFree(memory, 0, MEM_RELEASE);
    }
}

internal struct read_file_result
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
                             file_size.QuadPart,
                             &bytes_read,
                             NULL)
                    && (safe_truncate_uint64(file_size.QuadPart) == bytes_read))
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

internal void
win32_clear_sound_buffer(struct win32_sound_output *buffer)
{
    void *region1;
    DWORD region1_size;
    void *region2;
    DWORD region2_size;
    
    if (buffer->secondary_buffer->lpVtbl->Lock(buffer->secondary_buffer,
                                               0, buffer->secondary_buffer_size,
                                               &region1, &region1_size,
                                               &region2, &region2_size,
                                               0) == DS_OK)
    {
        u8 *dest_sample = (u8 *)region1;
        
        for (DWORD byte_index = 0; byte_index < region1_size; ++byte_index)
        {
            *dest_sample++ = 0;
        }
        
        dest_sample = (u8 *)region2;
        
        for (DWORD byte_index = 0; byte_index < region2_size; ++byte_index)
        {
            *dest_sample++ = 0;
        }
        
        buffer->secondary_buffer->lpVtbl->Unlock(buffer->secondary_buffer, region1, region1_size, region2, region2_size);
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
            Assert("Keyboard input came in through a non-dispatch message.");
        }break;
        
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
win32_fill_sound_buffer(struct win32_sound_output *sound_output, DWORD byte_to_lock, DWORD bytes_to_write, struct emulator_sound_output_buffer *source)
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
        
        DWORD region1_sample_count = region1_size / sound_output->bytes_per_sample;
        s16 *source_sample = source->samples;
        s16 *dest_sample = (s16 *)region1;
        
        for (DWORD sample_index = 0; sample_index < region1_sample_count; ++sample_index)
        {
            *dest_sample++ = *source_sample++;
            *dest_sample++ = *source_sample++;
            sound_output->running_sample_index++;
        }
        
        dest_sample = (s16 *)region2;
        DWORD region2_sample_count = region2_size / sound_output->bytes_per_sample;
        
        for (DWORD sample_index = 0; sample_index < region2_sample_count; ++sample_index)
        {
            *dest_sample++ = *source_sample++;
            *dest_sample++ = *source_sample++;
            sound_output->running_sample_index++;
        }
        
        sound_output->secondary_buffer->lpVtbl->Unlock(sound_output->secondary_buffer, region1, region1_size, region2, region2_size);
    }
}

internal void
win32_process_keyboard_message(struct emulator_button_state *new_state, b32 is_down)
{
    new_state->ended_down = is_down;
    ++new_state->half_transition_count;
}

internal void
win32_process_pending_messages(struct emulator_keyboard_input *input)
{
    MSG message;
    while(PeekMessageA(&message, 0, 0, 0, PM_REMOVE))
    {
        switch (message.message)
        {
            case WM_QUIT:
            {
                global_running = false;
            } break;
            
            case WM_SYSKEYDOWN:
            case WM_SYSKEYUP:
            case WM_KEYDOWN:
            case WM_KEYUP:
            {
                b32 is_down = ((message.lParam & (1 << 31)) == 0); // NOTE: The transition state. The value is always 0 for a WM_KEYDOWN message.
                b32 was_down = ((message.lParam & (1 << 30)) != 0); // NOTE: The previous key state. The value is 1 if the key is down before the message is sent, or it is zero if the key is up.
                u32 vk_code = message.wParam;
                
                if (is_down != was_down)
                {
                    b32 alt_key_was_down = ((message.lParam & (1 << 29)) != 0);
                    
                    if ((vk_code == VK_F4) && alt_key_was_down)
                    {
                        global_running = false;
                    }
                    else if (vk_code == '1')
                    {
                        win32_process_keyboard_message(&input->numeric_1, is_down);
                        
                        if (is_down)
                        {
                            OutputDebugString("1 is down");
                        }
                        
                        if (was_down)
                        {
                            OutputDebugString("1 was down");
                        }
                        
                    }
                    else if (vk_code == '2')
                    {
                        win32_process_keyboard_message(&input->numeric_2, is_down);
                    }
                    else if (vk_code == '3')
                    {
                        win32_process_keyboard_message(&input->numeric_3, is_down);
                    }
                    else if (vk_code == '4')
                    {
                        win32_process_keyboard_message(&input->numeric_4, is_down);
                    }
                    else if (vk_code == 'Q')
                    {
                        win32_process_keyboard_message(&input->Q, is_down);
                    }
                    else if (vk_code == 'W')
                    {
                        win32_process_keyboard_message(&input->W, is_down);
                    }
                    else if (vk_code == 'E')
                    {
                        win32_process_keyboard_message(&input->E, is_down);
                    }
                    else if (vk_code == 'R')
                    {
                        win32_process_keyboard_message(&input->R, is_down);
                    }
                    else if (vk_code == 'A')
                    {
                        win32_process_keyboard_message(&input->A, is_down);
                    }
                    else if (vk_code == 'S')
                    {
                        win32_process_keyboard_message(&input->S, is_down);
                    }
                    else if (vk_code == 'D')
                    {
                        win32_process_keyboard_message(&input->D, is_down);
                    }
                    else if (vk_code == 'F')
                    {
                        win32_process_keyboard_message(&input->F, is_down);
                    }
                    else if (vk_code == 'Z')
                    {
                        win32_process_keyboard_message(&input->Z, is_down);
                    }
                    else if (vk_code == 'X')
                    {
                        win32_process_keyboard_message(&input->X, is_down);
                    }
                    else if (vk_code == 'C')
                    {
                        win32_process_keyboard_message(&input->C, is_down);
                    }
                    else if (vk_code == 'V')
                    {
                        win32_process_keyboard_message(&input->V, is_down);
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