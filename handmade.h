#include "mydef.h"
#include <cstdint>

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

struct game_sound_output_buffer
{
    int32_t SamplesPerSecond;
    int32_t SampleCount;
    int16_t *Samples;
};

struct game_button_state
{
    int32_t HalfTransitionCount;
    bool32 EndedDown;
};
struct game_controller_input
{
    bool32 isAnalog;

    float32 StartX;
    float32 MaxX;
    float32 MinX;
    float32 EndX;

    float32 StartY;
    float32 MaxY;
    float32 MinY;
    float32 EndY;

    union
    {
        game_button_state Buttons[0];
        struct
        {
            game_button_state Up;
            game_button_state Down;
            game_button_state Left;
            game_button_state Right;
            game_button_state LeftShoulder;
            game_button_state RightShoulder;
        };
    };
};

struct game_input
{
    game_controller_input Controllers[4];
};

private_func void GameUpdateAndRender(game_input *Input,
                                      game_offscreen_buffer *Buffer,
                                      game_sound_output_buffer *SoundBuffer);

#define HANDMADE_H
#endif
