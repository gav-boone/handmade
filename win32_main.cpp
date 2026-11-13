// TODO: This is not finalized
//
// - save game location
// - handle to exec
// - asset loading path
// - threading
// - raw input
// - sleep/time begin
// - clipCursor
// - fullscreen support
// - wm set cursor
// - QueryCancelAutoplay
// - WM_ACTIVATEAPP
// - Blit speed go up
// - Hardware accel
// - GetKeyboardLayout
//
// to begin...

#include "handmade.cpp"

#include <cstdint>
#include <dsound.h>
#include <memoryapi.h>
#include <stdio.h>
#include <windows.h>
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
    int16_t ToneVolume;
    int32_t ToneHz;
    int32_t WavePeriod;
    int32_t BytesPerSample;
    int32_t SecondaryBufferSize;
    int32_t LatencySampleCount;
};

global_var win32_offscreen_buffer GlobalBackBuffer;
global_var bool32 GlobalRunning;
global_var int32_t GlobalXPosition = 0;
global_var int32_t GlobalYPosition = 0;
global_var LPDIRECTSOUNDBUFFER GlobalSecondaryBuffer;

#define X_INPUT_GET_STATE(name) DWORD WINAPI name(DWORD dwUserIndex, XINPUT_STATE *pState)
typedef X_INPUT_GET_STATE(x_input_get_state);
X_INPUT_GET_STATE(XInputGetStateStub) { return ERROR_DEVICE_NOT_CONNECTED; }
global_var x_input_get_state *XInputGetState_ = XInputGetStateStub;
#define XInputGetState XInputGetState_

#define X_INPUT_SET_STATE(name) DWORD WINAPI name(DWORD dwUserIndex, XINPUT_STATE *pState)
typedef X_INPUT_SET_STATE(x_input_set_state);
X_INPUT_SET_STATE(XInputSetStateStub) { return ERROR_DEVICE_NOT_CONNECTED; }
global_var x_input_set_state *XInputSetState_ = XInputSetStateStub;
#define XInputSetState XInputSetState_

#define DIRECT_SOUND_CREATE(name)                                                                  \
    HRESULT WINAPI name(LPCGUID pcGuidDevice, LPDIRECTSOUND *ppDS, LPUNKNOWN pUnkOuter)
typedef DIRECT_SOUND_CREATE(direct_sound_create);

private_func void
Win32LoadXInput(void)
{
    HMODULE XInputLibrary = LoadLibraryA("xinput1_4.dll");
    if (!XInputLibrary)
    {
        HMODULE XInputLibrary = LoadLibraryA("xinput9_1_0.dll");
    }

    if (!XInputLibrary)
    {
        HMODULE XInputLibrary = LoadLibraryA("xinput1_3.dll");
    }

    if (XInputLibrary)
    {
        XInputGetState = (x_input_get_state *)GetProcAddress(XInputLibrary, "XInputGetState");
        if (!XInputGetState)
        {
            XInputGetState = XInputGetStateStub;
        }
        XInputSetState = (x_input_set_state *)GetProcAddress(XInputLibrary, "XInputSetState");
        if (!XInputGetState)
        {
            XInputGetState = XInputGetStateStub;
        }
    }
}

private_func win32_window_dimensions
Win32GetWindowDimensions(HWND hWindow)
{
    win32_window_dimensions Result;

    RECT ClientRect;
    GetClientRect(hWindow, &ClientRect);
    Result.Width = ClientRect.right - ClientRect.left;
    Result.Height = ClientRect.bottom - ClientRect.top;

    return Result;
}

private_func void
Win32ResizeDIBSection(win32_offscreen_buffer *Buffer, int32_t Width, int32_t Height)
{

    if (Buffer->Memory)
    {
        VirtualFree(Buffer->Memory, 0, MEM_RELEASE);
    }

    Buffer->Width = Width;
    Buffer->Height = Height;
    Buffer->BytesPerPixel = 4;

    Buffer->Info.bmiHeader.biSize = sizeof(Buffer->Info.bmiHeader);
    Buffer->Info.bmiHeader.biWidth = Buffer->Width;
    Buffer->Info.bmiHeader.biHeight = -Buffer->Height;
    Buffer->Info.bmiHeader.biPlanes = 1;
    Buffer->Info.bmiHeader.biBitCount = 32;
    Buffer->Info.bmiHeader.biCompression = BI_RGB;

    int32_t BufferMemorySize = Buffer->BytesPerPixel * Width * Height;
    Buffer->Memory = VirtualAlloc(0, BufferMemorySize, MEM_COMMIT, PAGE_READWRITE);
    Buffer->Pitch = Width * Buffer->BytesPerPixel;
}

private_func void
Win32DisplayBufferInWindow(win32_offscreen_buffer *Buffer,
                           HDC DeviceContext,
                           int32_t WindowWidth,
                           int32_t WindowHeight)
{
    // TODO: aspect ratio correction
    StretchDIBits(DeviceContext,
                  0,
                  0,
                  WindowWidth,
                  WindowHeight,
                  0,
                  0,
                  Buffer->Width,
                  Buffer->Height,
                  Buffer->Memory,
                  &Buffer->Info,
                  DIB_RGB_COLORS,
                  SRCCOPY);
}

private_func void
Win32HandleKeyboardInputs(WPARAM WParam, WPARAM LParam)
{
    uint32_t VKCode = WParam;
    bool32 WasDown = ((LParam & (1 << 30)) != 0);
    bool32 IsDown = ((LParam & (1 << 31)) == 0);
    bool32 AltKeyWasDown = ((LParam & (1 << 29)) != 0);

    if (WasDown != IsDown)
    {
        switch (VKCode)
        {
        case ('W'):
        case (VK_UP):
        {
            GlobalYPosition += 8;
        }
        break;
        case ('A'):
        case (VK_LEFT):
        {
            GlobalXPosition += 8;
        }
        break;
        case ('S'):
        case (VK_DOWN):
        {
            GlobalYPosition -= 8;
        }
        break;
        case ('D'):
        case (VK_RIGHT):
        {
            GlobalXPosition -= 8;
        }
        break;
        case ('Q'):
        {
        }
        break;
        case ('E'):
        case (VK_ESCAPE):
        {
        }
        break;
        case (VK_SPACE):
        {
        }
        break;
        }

        if ((VKCode == VK_F4) && AltKeyWasDown)
        {
            GlobalRunning = false;
        }
    }
}

LRESULT CALLBACK
Win32MainWindowCallback(HWND hWindow, UINT Message, WPARAM WParam, LPARAM LParam)
{
    LRESULT Result = 0;

    switch (Message)
    {
    case WM_DESTROY:
    case WM_CLOSE:
        GlobalRunning = false;
        break;
    case WM_ACTIVATEAPP:
        break;
    case WM_SYSKEYDOWN:
    case WM_SYSKEYUP:
    case WM_KEYDOWN:
    case WM_KEYUP:
    {
        Win32HandleKeyboardInputs(WParam, LParam);
    }
    break;
    case WM_PAINT:
    {
        PAINTSTRUCT Paint;
        HDC DeviceContext = BeginPaint(hWindow, &Paint);
        win32_window_dimensions Dimensions = Win32GetWindowDimensions(hWindow);
        Win32DisplayBufferInWindow(&GlobalBackBuffer,
                                   DeviceContext,
                                   Dimensions.Width,
                                   Dimensions.Height);
        EndPaint(hWindow, &Paint);
        break;
    }
    default:
        Result = DefWindowProcA(hWindow, Message, WParam, LParam);
        break;
    }
    return Result;
}

private_func void
Win32HandleMessages()
{
    MSG Message;
    while (PeekMessageA(&Message, 0, 0, 0, PM_REMOVE))
    {
        if (Message.message == WM_QUIT)
        {
            GlobalRunning = false;
        }
        DispatchMessage(&Message);
    }
}

private_func void
Win32HandleControllerInputs()
{
    DWORD dwResult;
    for (DWORD ControllerIndex = 0; ControllerIndex < XUSER_MAX_COUNT; ++ControllerIndex)
    {

        XINPUT_STATE ControllerState;
        if (XInputGetState(ControllerIndex, &ControllerState) == ERROR_SUCCESS)
        {
            XINPUT_GAMEPAD *Pad = &ControllerState.Gamepad;

            bool32 DPadUp = (Pad->wButtons & XINPUT_GAMEPAD_DPAD_UP);
            bool32 DPadDown = (Pad->wButtons & XINPUT_GAMEPAD_DPAD_DOWN);
            bool32 DPadLeft = (Pad->wButtons & XINPUT_GAMEPAD_DPAD_LEFT);
            bool32 DPadRight = (Pad->wButtons & XINPUT_GAMEPAD_DPAD_RIGHT);
            bool32 Start = (Pad->wButtons & XINPUT_GAMEPAD_START);
            bool32 Back = (Pad->wButtons & XINPUT_GAMEPAD_BACK);
            bool32 Left_Shoulder = (Pad->wButtons & XINPUT_GAMEPAD_LEFT_SHOULDER);
            bool32 Right_Shoulder = (Pad->wButtons & XINPUT_GAMEPAD_RIGHT_SHOULDER);
            bool32 AButton = (Pad->wButtons & XINPUT_GAMEPAD_A);
            bool32 BButton = (Pad->wButtons & XINPUT_GAMEPAD_B);
            bool32 XButton = (Pad->wButtons & XINPUT_GAMEPAD_X);
            bool32 YButton = (Pad->wButtons & XINPUT_GAMEPAD_Y);

            int16_t StickX = Pad->sThumbLX;
            int16_t StickY = Pad->sThumbLY;
        }
    }
}

private_func void
Win32InitDSound(HWND hWindow, int32_t SamplePerSecond, int32_t BufferSize)
{
    // load lib
    HMODULE DSoundLibrary = LoadLibraryA("dsound.dll");

    if (DSoundLibrary)
    {
        // get dsound object
        direct_sound_create *DirectSoundCreate =
            (direct_sound_create *)GetProcAddress(DSoundLibrary, "DirectSoundCreate");

        LPDIRECTSOUND DirectSound;
        if (DirectSoundCreate && SUCCEEDED(DirectSoundCreate(0, &DirectSound, 0)))
        {
            WAVEFORMATEX WaveFormat = {};
            WaveFormat.wFormatTag = WAVE_FORMAT_PCM;
            WaveFormat.nChannels = 2;
            WaveFormat.nSamplesPerSec = SamplePerSecond;
            WaveFormat.wBitsPerSample = 16;
            WaveFormat.nBlockAlign = (WaveFormat.nChannels * WaveFormat.wBitsPerSample) / 8;
            WaveFormat.nAvgBytesPerSec = WaveFormat.nSamplesPerSec * WaveFormat.nBlockAlign;
            WaveFormat.cbSize = 0;
            if (SUCCEEDED(DirectSound->SetCooperativeLevel(hWindow, DSSCL_PRIORITY)))
            {
                DSBUFFERDESC BufferDescription = {};
                BufferDescription.dwSize = sizeof(BufferDescription);
                BufferDescription.dwFlags = DSBCAPS_PRIMARYBUFFER;

                // create primary buffer
                LPDIRECTSOUNDBUFFER PrimaryBuffer;
                if (SUCCEEDED(
                        DirectSound->CreateSoundBuffer(&BufferDescription, &PrimaryBuffer, 0)))
                {
                    if (SUCCEEDED(PrimaryBuffer->SetFormat(&WaveFormat)))
                    {
                        OutputDebugStringA("Primary Buffer was set. \n");
                    }
                    else
                    {
                    }
                }
            }
            else
            {
            }
            // create secondary buffer
            DSBUFFERDESC BufferDescription = {};
            BufferDescription.dwSize = sizeof(BufferDescription);
            BufferDescription.dwFlags = 0;
            BufferDescription.dwBufferBytes = BufferSize;
            BufferDescription.lpwfxFormat = &WaveFormat;

            if (SUCCEEDED(
                    DirectSound->CreateSoundBuffer(&BufferDescription, &GlobalSecondaryBuffer, 0)))
            {
                // start playing
                OutputDebugStringA("SecondaryBuffer was set. \n");
            }
        }
        else
        {
        }
    }
    else
    {
    }
}
void
Win32ClearSoundBuffer(win32_sound_output *SoundOutput)
{
    VOID *Region1;
    DWORD Region1Size;
    VOID *Region2;
    DWORD Region2Size;
    if (SUCCEEDED(GlobalSecondaryBuffer->Lock(0,
                                              SoundOutput->SecondaryBufferSize,
                                              &Region1,
                                              &Region1Size,
                                              &Region2,
                                              &Region2Size,
                                              0)))
    {
        uint8_t *DestSample = (uint8_t *)Region1;
        for (DWORD ByteIndex = 0; ByteIndex < Region1Size; ++ByteIndex)
        {
            *DestSample++ = 0;
        }
        DestSample = (uint8_t *)Region2;
        for (DWORD ByteIndex = 0; ByteIndex < Region2Size; ++ByteIndex)
        {
            *DestSample++ = 0;
        }
        GlobalSecondaryBuffer->Unlock(Region1, Region1Size, Region2, Region2Size);
    }
}

void
Win32FillSoundBuffer(win32_sound_output *SoundOutput,
                     DWORD ByteToLock,
                     DWORD BytesToWrite,
                     game_sound_output_buffer *SourceBuffer)
{
    VOID *Region1;
    DWORD Region1Size;
    VOID *Region2;
    DWORD Region2Size;
    if (SUCCEEDED(GlobalSecondaryBuffer->Lock(ByteToLock,
                                              BytesToWrite,
                                              &Region1,
                                              &Region1Size,
                                              &Region2,
                                              &Region2Size,
                                              0)))
    {

        int16_t *DestSample = (int16_t *)Region1;
        int16_t *SrcSample = SourceBuffer->Samples;
        DWORD Region1SampleCount = Region1Size / SoundOutput->BytesPerSample;
        for (DWORD SampleIndex = 0; SampleIndex < Region1SampleCount; ++SampleIndex)
        {
            *DestSample++ = *SrcSample++;
            *DestSample++ = *SrcSample++;
            ++SoundOutput->RunningSampleIndex;
        }
        DestSample = (int16_t *)Region2;
        DWORD Region2SampleCount = Region2Size / SoundOutput->BytesPerSample;
        for (DWORD SampleIndex = 0; SampleIndex < Region2SampleCount; ++SampleIndex)
        {
            *DestSample++ = *SrcSample++;
            *DestSample++ = *SrcSample++;
            ++SoundOutput->RunningSampleIndex;
        }
        GlobalSecondaryBuffer->Unlock(Region1, Region1Size, Region2, Region2Size);
    }
}

int32_t CALLBACK
WinMain(HINSTANCE hInstance, HINSTANCE hPreviousInstance, LPSTR lpCmdLine, int32_t CmdShow)
{
    LARGE_INTEGER PerfFreq;
    QueryPerformanceFrequency(&PerfFreq);
    int64_t PerfCountFreq = PerfFreq.QuadPart;

    Win32LoadXInput();

    WNDCLASSA WindowClass = {};

    Win32ResizeDIBSection(&GlobalBackBuffer, 1280, 720);

    WindowClass.style = CS_HREDRAW | CS_VREDRAW;
    WindowClass.lpfnWndProc = Win32MainWindowCallback;
    WindowClass.hInstance = hInstance;
    /*WindowClass.icon;*/
    WindowClass.lpszClassName = "HandmadeHeroWindowClass";

    if (RegisterClass(&WindowClass))
    {
        HWND hWindow = CreateWindowEx(0,
                                      WindowClass.lpszClassName,
                                      "Handmade Hero",
                                      WS_OVERLAPPEDWINDOW | WS_VISIBLE,
                                      CW_USEDEFAULT,
                                      CW_USEDEFAULT,
                                      CW_USEDEFAULT,
                                      CW_USEDEFAULT,
                                      0,
                                      0,
                                      hInstance,
                                      0);

        if (hWindow)
        {
            win32_sound_output SoundOutput = {};

            SoundOutput.SamplesPerSecond = 48000;
            SoundOutput.RunningSampleIndex = 0;
            SoundOutput.ToneVolume = 500;
            SoundOutput.ToneHz = 256;
            SoundOutput.WavePeriod = SoundOutput.SamplesPerSecond / SoundOutput.ToneHz;
            SoundOutput.BytesPerSample = sizeof(int16_t) * 2;
            SoundOutput.SecondaryBufferSize =
                SoundOutput.SamplesPerSecond * SoundOutput.BytesPerSample;

            HDC DeviceContext = GetDC(hWindow);

            Win32InitDSound(hWindow, SoundOutput.SamplesPerSecond, SoundOutput.SecondaryBufferSize);
            Win32ClearSoundBuffer(&SoundOutput);
            GlobalSecondaryBuffer->Play(0, 0, DSBPLAY_LOOPING);

            int16_t *Samples = (int16_t *)
                VirtualAlloc(0, SoundOutput.SecondaryBufferSize, MEM_COMMIT, PAGE_READWRITE);

            LARGE_INTEGER LastCounter;
            QueryPerformanceCounter(&LastCounter);

            uint64_t LastCycleCount = __rdtsc();

            GlobalRunning = true;
            while (GlobalRunning)
            {
                Win32HandleMessages();

                Win32HandleControllerInputs();

                DWORD ByteToLock = 0;
                DWORD BytesToWrite = 0;
                DWORD TargetCursor = 0;
                DWORD PlayCursor = 0;
                DWORD WriteCursor = 0;
                bool32 SoundIsValid = false;
                if (SUCCEEDED(GlobalSecondaryBuffer->GetCurrentPosition(&PlayCursor, &WriteCursor)))
                {
                    ByteToLock = (SoundOutput.RunningSampleIndex * SoundOutput.BytesPerSample) %
                                 SoundOutput.SecondaryBufferSize;
                    TargetCursor = (PlayCursor +
                                    (SoundOutput.LatencySampleCount * SoundOutput.BytesPerSample)) %
                                   SoundOutput.SecondaryBufferSize;
                    if (ByteToLock > TargetCursor)
                    {
                        BytesToWrite = SoundOutput.SecondaryBufferSize - ByteToLock;
                        BytesToWrite += TargetCursor;
                    }
                    else
                    {
                        BytesToWrite = TargetCursor - ByteToLock;
                    }
                    SoundIsValid = true;
                }

                game_sound_output_buffer SoundBuffer = {};
                SoundBuffer.SamplesPerSecond = SoundOutput.SamplesPerSecond;
                SoundBuffer.SampleCount = BytesToWrite / SoundOutput.BytesPerSample;
                SoundBuffer.Samples = Samples;

                game_offscreen_buffer Buffer = {};
                Buffer.Memory = GlobalBackBuffer.Memory;
                Buffer.Width = GlobalBackBuffer.Width;
                Buffer.Height = GlobalBackBuffer.Height;
                Buffer.Pitch = GlobalBackBuffer.Pitch;
                Buffer.BytesPerPixel = GlobalBackBuffer.BytesPerPixel;
                GameUpdateAndRender(&Buffer,
                                    GlobalXPosition,
                                    GlobalYPosition,
                                    &SoundBuffer,
                                    SoundOutput.ToneHz);

                if (SoundIsValid)
                {
                    Win32FillSoundBuffer(&SoundOutput, ByteToLock, BytesToWrite, &SoundBuffer);
                }

                win32_window_dimensions Dimensions = Win32GetWindowDimensions(hWindow);
                Win32DisplayBufferInWindow(&GlobalBackBuffer,
                                           DeviceContext,
                                           Dimensions.Width,
                                           Dimensions.Height);
                LARGE_INTEGER EndCounter;
                QueryPerformanceCounter(&EndCounter);

                uint64_t EndCycleCount = __rdtsc();

                uint64_t CyclesElapsed = EndCycleCount - LastCycleCount;
                int64_t CounterElapsed = EndCounter.QuadPart - LastCounter.QuadPart;
                float64 MsPerFrame = (1000.0f * (float64)CounterElapsed / (float64)PerfCountFreq);
                float64 FramesPerSec = (float64)PerfCountFreq / (float64)CounterElapsed;
                float64 MegaCycles = (float64)CyclesElapsed / (1000.0f * 1000.0f);

                char PerfBuffer[256];
                sprintf(PerfBuffer,
                        "Ms/Frame: %f  |  FPS: %f  | Cycles: %f\n",
                        MsPerFrame,
                        FramesPerSec,
                        MegaCycles);
                OutputDebugStringA(PerfBuffer);

                LastCounter = EndCounter;
                LastCycleCount = EndCycleCount;
            }
        }
    }
    else
    {
        // TODO: logging
    };

    return 0;
}
