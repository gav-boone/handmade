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

private_func void Win32ResizeDIBSection(int Width, int Height) {

    if (BitmapMemory) {
        VirtualFree(BitmapMemory, 0, MEM_RELEASE);
    }

    BitmapInfo.bmiHeader.biSize = sizeof(BitmapInfo.bmiHeader);
    BitmapInfo.bmiHeader.biWidth = Width;
    BitmapInfo.bmiHeader.biHeight = Height;
    BitmapInfo.bmiHeader.biPlanes = 1;
    BitmapInfo.bmiHeader.biBitCount = 32;
    BitmapInfo.bmiHeader.biCompression = BI_RGB;

    int BytesPerPixel = 4;
    int BitmapMemorySize = BytesPerPixel * Width * Height;
    BitmapMemory =
        VirtualAlloc(0, BitmapMemorySize, MEM_COMMIT, PAGE_READWRITE);
}

private_func void Win32UpdateWindow(HDC DeviceContext, int X, int Y, int Width,
                                    int Height) {
    StretchDIBits(DeviceContext, X, Y, Width, Height, X, Y, Width, Height,
                  BitmapMemory, &BitmapInfo, DIB_RGB_COLORS, SRCCOPY);
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

        int Height = Paint.rcPaint.bottom - Paint.rcPaint.top;
        int Width = Paint.rcPaint.right - Paint.rcPaint.left;
        int X = Paint.rcPaint.left;
        int Y = Paint.rcPaint.top;

        Win32UpdateWindow(DeviceContext, X, Y, Width, Height);

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
            while (Running) {
                MSG Message;
                BOOL MessageResult = GetMessage(&Message, 0, 0, 0);
                if (MessageResult > 0) {
                    DispatchMessage(&Message);
                } else {
                    break;
                }
            }
        } else {
            // TODO: logging
        }
    } else {
        // TODO: logging
    };

    return 0;
}
