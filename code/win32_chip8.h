/* date = July 13th 2024 6:34 am */

#ifndef WIN32_CHIP8_H
#define WIN32_CHIP8_H

#define internal static
#define local_persist static
#define global_variable static

#define Pi32 3.14159265359f

typedef uint8_t u8; // TODO: get rid of these
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
};

struct win32_sound_output
{
    u32 running_sample_index;
    int secondary_buffer_size;
    int bytes_per_sample;
    int samples_per_second;
    IDirectSoundBuffer *secondary_buffer;
    int latency_sample_count;
};

struct win32_window_dimension
{
    int width;
    int height;
};

internal void
win32_clear_sound_buffer(struct win32_sound_output *buffer);

internal void
win32_display_buffer_in_window(struct win32_offscreen_buffer *buffer, HDC device_context, int window_width, int window_height);

internal struct win32_window_dimension
win32_get_window_dimension(HWND window);

internal void
win32_fill_sound_buffer(struct win32_sound_output *sound_output, DWORD byte_to_lock, DWORD bytes_to_write, struct emulator_sound_output_buffer *source);

internal void
win32_init_d_sound(HWND window, struct win32_sound_output *sound_output);

internal void
win32_init_x_audio(void);

internal void
win32_process_keyboard_message(struct emulator_button_state *new_state, b32 is_down);

internal void
win32_process_pending_messages(struct emulator_keyboard_input *input);

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

#endif //WIN32_CHIP8_H
