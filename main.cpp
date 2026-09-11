#include <cstdint>
#include <memoryapi.h>
#include <windef.h>
#include <windows.h>
#include <wingdi.h>
#include <winnt.h>

#define private_func static
#define local_persist static
#define global_var static

global_var bool Running;

global_var BITMAPINFO BitmapInfo;
global_var void *BitmapMemory;
global_var int BitmapWidth;
global_var int BitmapHeight;
global_var int BytesPerPixel = 4;

private_func void RenderWeirdGradient(int XOffset, int YOffset) {
    int Pitch = BitmapWidth * BytesPerPixel;
    uint8_t *Row = (uint8_t *)BitmapMemory;
    for (int Y = 0; Y < BitmapHeight; ++Y) {
        uint8_t *Pixel = (uint8_t *)Row;
        for (int X = 0; X < BitmapWidth; ++X) {
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
        Row += Pitch;
    }
}

private_func void Win32ResizeDIBSection(int Width, int Height) {

    if (BitmapMemory) {
        VirtualFree(BitmapMemory, 0, MEM_RELEASE);
    }

    BitmapWidth = Width;
    BitmapHeight = Height;

    BitmapInfo.bmiHeader.biSize = sizeof(BitmapInfo.bmiHeader);
    BitmapInfo.bmiHeader.biWidth = BitmapWidth;
    BitmapInfo.bmiHeader.biHeight = -BitmapHeight;
    BitmapInfo.bmiHeader.biPlanes = 1;
    BitmapInfo.bmiHeader.biBitCount = 32;
    BitmapInfo.bmiHeader.biCompression = BI_RGB;

    int BitmapMemorySize = BytesPerPixel * Width * Height;
    BitmapMemory =
        VirtualAlloc(0, BitmapMemorySize, MEM_COMMIT, PAGE_READWRITE);
}

private_func void Win32UpdateWindow(HDC DeviceContext, RECT *ClientRect) {
    int WindowWidth = ClientRect->right - ClientRect->left;
    int WindowHeight = ClientRect->bottom - ClientRect->top;

    StretchDIBits(DeviceContext, 0, 0, BitmapWidth, BitmapHeight, 0, 0,
                  WindowWidth, WindowHeight, BitmapMemory, &BitmapInfo,
                  DIB_RGB_COLORS, SRCCOPY);
}

LRESULT CALLBACK Win32MainWindowCallback(HWND hWindow, UINT Message,
                                         WPARAM WParam, LPARAM LParam) {
    LRESULT Result = 0;

    switch (Message) {
    case WM_SIZE: {
        RECT ClientRect;
        GetClientRect(hWindow, &ClientRect);
        int Height = ClientRect.bottom - ClientRect.top;
        int Width = ClientRect.right - ClientRect.left;
        Win32ResizeDIBSection(Width, Height);
    } break;
    case WM_DESTROY:
    case WM_CLOSE:
        Running = false;
        break;
    case WM_ACTIVATEAPP:
        break;
    case WM_PAINT: {
        PAINTSTRUCT Paint;
        HDC DeviceContext = BeginPaint(hWindow, &Paint);
        RECT ClientRect;
        GetClientRect(hWindow, &ClientRect);
        Win32UpdateWindow(DeviceContext, &ClientRect);

        EndPaint(hWindow, &Paint);
        break;
    }
    default:
        Result = DefWindowProc(hWindow, Message, WParam, LParam);
        break;
    }
    return Result;
}

int CALLBACK WinMain(HINSTANCE hInstance, HINSTANCE hPreviousInstance,
                     LPSTR lpCmdLine, int CmdShow) {

    WNDCLASS WindowClass = {};

    WindowClass.style = CS_OWNDC | CS_HREDRAW | CS_VREDRAW;
    WindowClass.lpfnWndProc = Win32MainWindowCallback;
    WindowClass.hInstance = hInstance;
    /*WindowClass.icon;*/
    WindowClass.lpszClassName = "HandmadeHeroWindowClass";

    if (RegisterClass(&WindowClass)) {
        HWND hWindow = CreateWindowEx(
            0, WindowClass.lpszClassName, "Handmade Hero",
            WS_OVERLAPPEDWINDOW | WS_VISIBLE, CW_USEDEFAULT, CW_USEDEFAULT,
            CW_USEDEFAULT, CW_USEDEFAULT, 0, 0, hInstance, 0);

        if (hWindow) {
            Running = true;
            int XOffset = 0;
            int YOffset = 0;

            while (Running) {
                MSG Message;
                while (PeekMessageA(&Message, 0, 0, 0, PM_REMOVE)) {
                    if (Message.message == WM_QUIT) {
                        Running = false;
                    }
                    DispatchMessage(&Message);
                }
                RenderWeirdGradient(XOffset, YOffset);
                ++XOffset;
                ++YOffset;

                HDC DeviceContext = GetDC(hWindow);
                RECT ClientRect;
                GetClientRect(hWindow, &ClientRect);
                Win32UpdateWindow(DeviceContext, &ClientRect);
                ReleaseDC(hWindow, DeviceContext);
            }
        } else {
            // TODO: logging
        }
    } else {
        // TODO: logging
    };

    return 0;
}
