#include <cstdint>
#include <memoryapi.h>
#include <windef.h>
#include <windows.h>
#include <wingdi.h>
#include <winnt.h>

#define private_func static
#define local_persist static
#define global_var static

struct win32_offscreen_buffer
{
    BITMAPINFO Info;
    void *Memory;
    int Width;
    int Height;
    int Pitch;
    int BytesPerPixel;
};

global_var win32_offscreen_buffer GlobalBackBuffer;
global_var bool Running;

struct win32_window_dimensions
{
    int Width;
    int Height;
};

win32_window_dimensions GetWindowDimensions(HWND hWindow)
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

            *Pixel = (uint8_t)(X + XOffset + Y + YOffset);
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
    Buffer->Memory =
        VirtualAlloc(0, BufferMemorySize, MEM_COMMIT, PAGE_READWRITE);
    Buffer->Pitch = Width * Buffer->BytesPerPixel;
}

private_func void Win32DisplayBufferInWindow(win32_offscreen_buffer Buffer,
                                             HDC DeviceContext,
                                             win32_window_dimensions Dimensions)
{

    StretchDIBits(DeviceContext,
                  0,
                  0,
                  Buffer.Width,
                  Buffer.Height,
                  0,
                  0,
                  Dimensions.Width,
                  Dimensions.Height,
                  Buffer.Memory,
                  &Buffer.Info,
                  DIB_RGB_COLORS,
                  SRCCOPY);
}

LRESULT CALLBACK Win32MainWindowCallback(HWND hWindow,
                                         UINT Message,
                                         WPARAM WParam,
                                         LPARAM LParam)
{
    LRESULT Result = 0;

    switch (Message)
    {
    case WM_SIZE:
    {
        win32_window_dimensions Dimensions = GetWindowDimensions(hWindow);
        Win32ResizeDIBSection(&GlobalBackBuffer,
                              Dimensions.Width,
                              Dimensions.Height);
    }
    break;
    case WM_DESTROY:
    case WM_CLOSE:
        Running = false;
        break;
    case WM_ACTIVATEAPP:
        break;
    case WM_PAINT:
    {
        PAINTSTRUCT Paint;
        HDC DeviceContext = BeginPaint(hWindow, &Paint);
        win32_window_dimensions Dimensions = GetWindowDimensions(hWindow);
        Win32DisplayBufferInWindow(GlobalBackBuffer, DeviceContext, Dimensions);
        EndPaint(hWindow, &Paint);
        break;
    }
    default:
        Result = DefWindowProc(hWindow, Message, WParam, LParam);
        break;
    }
    return Result;
}

int CALLBACK WinMain(HINSTANCE hInstance,
                     HINSTANCE hPreviousInstance,
                     LPSTR lpCmdLine,
                     int CmdShow)
{

    WNDCLASS WindowClass = {};

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
            int XOffset = 0;
            int YOffset = 0;

            Running = true;
            while (Running)
            {
                MSG Message;
                while (PeekMessageA(&Message, 0, 0, 0, PM_REMOVE))
                {
                    if (Message.message == WM_QUIT)
                    {
                        Running = false;
                    }
                    DispatchMessage(&Message);
                }
                RenderWeirdGradient(GlobalBackBuffer, XOffset, YOffset);
                ++XOffset;
                ++YOffset;

                HDC DeviceContext = GetDC(hWindow);
                win32_window_dimensions Dimensions =
                    GetWindowDimensions(hWindow);
                Win32DisplayBufferInWindow(GlobalBackBuffer,
                                           DeviceContext,
                                           Dimensions);
                ReleaseDC(hWindow, DeviceContext);
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
