#include <dsound.h>
#include <math.h>
#include <stdint.h>
#include <windows.h>
#include <xinput.h>

#define private_func static
#define local_persist static
#define global_var static
typedef double float64;
typedef float float32;
#define PI32 3.14159274101257324f

struct win32_offscreen_buffer
{
    BITMAPINFO Info;
    void *Memory;
    int Width;
    int Height;
    int Pitch;
    int BytesPerPixel;
};

struct win32_window_dimensions
{
    int Width;
    int Height;
};

struct win32_sound_output
{
    int SamplePerSecond;
    int32_t RunningSampleIndex;
    int16_t ToneVolume;
    int WavePeriod;
    int BytesPerSample;
    int SecondaryBufferSize;
    int LatencySampleCount;
};

global_var win32_offscreen_buffer GlobalBackBuffer;
global_var bool GlobalRunning;
global_var int GlobalXPosition = 0;
global_var int GlobalYPosition = 0;
global_var LPDIRECTSOUNDBUFFER GlobalSecondaryBuffer;

// NOTE: XInputGetState support
#define X_INPUT_GET_STATE(name) DWORD WINAPI name(DWORD dwUserIndex, XINPUT_STATE *pState)
typedef X_INPUT_GET_STATE(x_input_get_state);
X_INPUT_GET_STATE(XInputGetStateStub) { return ERROR_DEVICE_NOT_CONNECTED; }
global_var x_input_get_state *XInputGetState_ = XInputGetStateStub;
#define XInputGetState XInputGetState_

// NOTE: XInputSetState support
#define X_INPUT_SET_STATE(name) DWORD WINAPI name(DWORD dwUserIndex, XINPUT_STATE *pState)
typedef X_INPUT_SET_STATE(x_input_set_state);
X_INPUT_SET_STATE(XInputSetStateStub) { return ERROR_DEVICE_NOT_CONNECTED; }
global_var x_input_set_state *XInputSetState_ = XInputSetStateStub;
#define XInputSetState XInputSetState_

// NOTE: XInputSetState support
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
RenderWeirdGradient(win32_offscreen_buffer Buffer, int XOffset, int YOffset)
{
    uint8_t *Row = (uint8_t *)Buffer.Memory;
    for (int Y = 0; Y < Buffer.Height; ++Y)
    {
        uint8_t *Pixel = (uint8_t *)Row;
        for (int X = 0; X < Buffer.Width; ++X)
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
        Row += Buffer.Pitch;
    }
}

private_func void
Win32ResizeDIBSection(win32_offscreen_buffer *Buffer, int Width, int Height)
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

    int BufferMemorySize = Buffer->BytesPerPixel * Width * Height;
    Buffer->Memory = VirtualAlloc(0, BufferMemorySize, MEM_COMMIT, PAGE_READWRITE);
    Buffer->Pitch = Width * Buffer->BytesPerPixel;
}

private_func void
Win32DisplayBufferInWindow(win32_offscreen_buffer *Buffer,
                           HDC DeviceContext,
                           int WindowWidth,
                           int WindowHeight)
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
    bool WasDown = ((LParam & (1 << 30)) != 0);
    bool IsDown = ((LParam & (1 << 31)) == 0);
    bool AltKeyWasDown = ((LParam & (1 << 29)) != 0);

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

            bool DPadUp = (Pad->wButtons & XINPUT_GAMEPAD_DPAD_UP);
            bool DPadDown = (Pad->wButtons & XINPUT_GAMEPAD_DPAD_DOWN);
            bool DPadLeft = (Pad->wButtons & XINPUT_GAMEPAD_DPAD_LEFT);
            bool DPadRight = (Pad->wButtons & XINPUT_GAMEPAD_DPAD_RIGHT);
            bool Start = (Pad->wButtons & XINPUT_GAMEPAD_START);
            bool Back = (Pad->wButtons & XINPUT_GAMEPAD_BACK);
            bool Left_Shoulder = (Pad->wButtons & XINPUT_GAMEPAD_LEFT_SHOULDER);
            bool Right_Shoulder = (Pad->wButtons & XINPUT_GAMEPAD_RIGHT_SHOULDER);
            bool AButton = (Pad->wButtons & XINPUT_GAMEPAD_A);
            bool BButton = (Pad->wButtons & XINPUT_GAMEPAD_B);
            bool XButton = (Pad->wButtons & XINPUT_GAMEPAD_X);
            bool YButton = (Pad->wButtons & XINPUT_GAMEPAD_Y);

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
Win32FillSoundBuffer(win32_sound_output *SoundOutput, DWORD ByteToLock, DWORD BytesToWrite)
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

        int16_t *SampleOut = (int16_t *)Region1;
        DWORD Region1SampleCount = Region1Size / SoundOutput->BytesPerSample;
        for (DWORD SampleIndex = 0; SampleIndex < Region1SampleCount; ++SampleIndex)
        {
            float32 t =
                2.0f * PI32 *
                ((float32)SoundOutput->RunningSampleIndex / (float32)SoundOutput->WavePeriod);
            float32 SineValue = sinf(t);
            int16_t SampleValue = (uint16_t)(SineValue * SoundOutput->ToneVolume);
            *SampleOut++ = SampleValue;
            *SampleOut++ = SampleValue;

            ++SoundOutput->RunningSampleIndex;
        }
        SampleOut = (int16_t *)Region2;
        DWORD Region2SampleCount = Region2Size / SoundOutput->BytesPerSample;
        for (DWORD SampleIndex = 0; SampleIndex < Region2SampleCount; ++SampleIndex)
        {
            float32 t =
                2.0f * PI32 *
                ((float32)SoundOutput->RunningSampleIndex / (float32)SoundOutput->WavePeriod);
            float32 SineValue = sinf(t);
            int16_t SampleValue = (uint16_t)(SineValue * SoundOutput->ToneVolume);
            *SampleOut++ = SampleValue;
            *SampleOut++ = SampleValue;

            ++SoundOutput->RunningSampleIndex;
        }
        GlobalSecondaryBuffer->Unlock(Region1, Region1Size, Region2, Region2Size);
    }
}

int CALLBACK
WinMain(HINSTANCE hInstance, HINSTANCE hPreviousInstance, LPSTR lpCmdLine, int CmdShow)
{
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

            SoundOutput.SamplePerSecond = 48000;
            SoundOutput.RunningSampleIndex = 0;
            SoundOutput.ToneVolume = 500;
            SoundOutput.WavePeriod = SoundOutput.SamplePerSecond / 256;
            SoundOutput.BytesPerSample = sizeof(int16_t) * 2;
            SoundOutput.SecondaryBufferSize =
                SoundOutput.SamplePerSecond * SoundOutput.BytesPerSample;

            HDC DeviceContext = GetDC(hWindow);

            Win32InitDSound(hWindow, SoundOutput.SamplePerSecond, SoundOutput.SecondaryBufferSize);
            Win32FillSoundBuffer(&SoundOutput, 0, SoundOutput.SecondaryBufferSize);
            GlobalSecondaryBuffer->Play(0, 0, DSBPLAY_LOOPING);

            GlobalRunning = true;
            while (GlobalRunning)
            {
                Win32HandleMessages();

                Win32HandleControllerInputs();

                RenderWeirdGradient(GlobalBackBuffer, GlobalXPosition, GlobalYPosition);

                // Direct Sound Output Test
                DWORD PlayCursor;
                DWORD WriteCuror;
                if (SUCCEEDED(GlobalSecondaryBuffer->GetCurrentPosition(&PlayCursor, &WriteCuror)))
                {
                    DWORD ByteToLock =
                        (SoundOutput.RunningSampleIndex * SoundOutput.BytesPerSample) %
                        SoundOutput.SecondaryBufferSize;
                    DWORD BytesToWrite;
                    if (ByteToLock > PlayCursor)
                    {
                        BytesToWrite = SoundOutput.SecondaryBufferSize - ByteToLock;
                        BytesToWrite += PlayCursor;
                    }
                    else
                    {
                        BytesToWrite = PlayCursor - ByteToLock;
                    }

                    Win32FillSoundBuffer(&SoundOutput, ByteToLock, BytesToWrite);
                }

                win32_window_dimensions Dimensions = Win32GetWindowDimensions(hWindow);
                Win32DisplayBufferInWindow(&GlobalBackBuffer,
                                           DeviceContext,
                                           Dimensions.Width,
                                           Dimensions.Height);
            }
        }
        else
        {
            // TODO: logging
        }
    }
    else
    {
        // TODO: logging
    };

    return 0;
}
