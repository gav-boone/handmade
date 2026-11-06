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
    int Width;
    int Height;
    int Pitch;
    int BytesPerPixel;
};
private_func void GameUpdateAndRender(game_offscreen_buffer *Buffer, int XOffset, int YOffset);

#define HANDMADE_H
#endif
