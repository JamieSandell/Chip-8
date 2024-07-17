/* date = July 13th 2024 6:34 am */

#ifndef WIN32_CHIP8_H
#define WIN32_CHIP8_H

#define internal static
#define local_persist static
#define global_variable static

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

#endif //WIN32_CHIP8_H
