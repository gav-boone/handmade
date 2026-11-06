#include "handmade.h"
#include <stdint.h>

#define private_func static

private_func void
RenderWeirdGradient(game_offscreen_buffer *Buffer, int XOffset, int YOffset)
{
    uint8_t *Row = (uint8_t *)Buffer->Memory;
    for (int Y = 0; Y < Buffer->Height; ++Y)
    {
        uint8_t *Pixel = (uint8_t *)Row;
        for (int X = 0; X < Buffer->Width; ++X)
        {
            /*
                Pixel in mem:
                    BB GG RR xx
                    0x xxRRGGBB
            */

            *Pixel = (uint8_t)(X + XOffset);
            ++Pixel;

            *Pixel = (uint8_t)(Y + YOffset);
            ++Pixel;

            *Pixel = (uint8_t)((X + XOffset + Y + YOffset) / 2);
            ++Pixel;

            *Pixel = 0;
            ++Pixel;
        }
        Row += Buffer->Pitch;
    }
}

private_func void
GameUpdateAndRender(game_offscreen_buffer *Buffer, int XOffset, int YOffset)
{
    RenderWeirdGradient(Buffer, XOffset, YOffset);
}
