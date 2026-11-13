#if !defined(WIN32_HANDMADE_H)

#include "mydef.h"
#include <windows.h>

#include <dsound.h>
#include <memoryapi.h>
#include <xinput.h>

struct win32_offscreen_buffer
{
    BITMAPINFO Info;
    void *Memory;
    int32_t Width;
    int32_t Height;
    int32_t Pitch;
    int32_t BytesPerPixel;
};

struct win32_window_dimensions
{
    int32_t Width;
    int32_t Height;
};

struct win32_sound_output
{
    int32_t SamplesPerSecond;
    int32_t RunningSampleIndex;
    int32_t BytesPerSample;
    int32_t SecondaryBufferSize;
    int32_t LatencySampleCount;
};

#define WIN32_HANDMADE_H
#endif
