#include "handmade.h"
#include <cstdint>
#include <math.h>

void
OutputGameSound(game_sound_output_buffer *SoundBuffer, int32_t ToneHz)
{
    local_persist float32 tSine;
    int16_t ToneVolume = 500;
    int32_t WavePeriod = SoundBuffer->SamplesPerSecond / ToneHz;

    int16_t *SampleOut = SoundBuffer->Samples;
    for (int32_t SampleIndex = 0; SampleIndex < SoundBuffer->SampleCount; ++SampleIndex)
    {
        float32 SineValue = sinf(tSine);
        int16_t SampleValue = (int16_t)(SineValue * ToneVolume);
        *SampleOut++ = SampleValue;
        *SampleOut++ = SampleValue;

        tSine += 2.0f * PI32 / ((float32)WavePeriod);
    }
}

private_func void
RenderWeirdGradient(game_offscreen_buffer *Buffer, int32_t XOffset, int32_t YOffset)
{
    uint8_t *Row = (uint8_t *)Buffer->Memory;
    for (int32_t Y = 0; Y < Buffer->Height; ++Y)
    {
        uint8_t *Pixel = (uint8_t *)Row;
        for (int32_t X = 0; X < Buffer->Width; ++X)
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
GameUpdateAndRender(game_offscreen_buffer *Buffer,
                    int32_t XOffset,
                    int32_t YOffset,
                    game_sound_output_buffer *SoundBuffer,
                    int32_t ToneHz)
{
    // TODO: add bias to move sound relative to generation
    OutputGameSound(SoundBuffer, ToneHz);
    RenderWeirdGradient(Buffer, XOffset, YOffset);
}
