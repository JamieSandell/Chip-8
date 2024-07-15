/* date = July 15th 2024 7:11 am */

#ifndef CHIP8_H
#define CHIP8_H

// TODO: Services that the platform layer provides to the emulator.

// NOTE: Services that the emulator provides to the platform layer.
void
emulator_update_and_render(void);

internal void
render_weird_gradient(struct win32_offscreen_buffer *buffer, int x_offset, int y_offset);

#endif //CHIP8_H
