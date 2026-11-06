#include "mydef.h"

#if !defined(HANDMADE_H)
// Services platform -> game

// Servicer game -> platform
// What game needs:
// 1. User Input
// 2. Timing
// 3. Bitmap Buffer
// 4. Sound Buffer
struct game_offscreen_buffer
{
    void *Memory;
    int32_t Width;
    int32_t Height;
    int32_t Pitch;
    int32_t BytesPerPixel;
};
private_func void
GameUpdateAndRender(game_offscreen_buffer *Buffer, int32_t XOffset, int32_t YOffset);

#define HANDMADE_H
#endif
