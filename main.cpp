#include <minwindef.h>
#include <windows.h>
#include <wingdi.h>

LRESULT CALLBACK MainWindowCallback(HWND hWindow, UINT Message, WPARAM WParam,
                                    LPARAM LParam) {
    LRESULT Result = 0;

    switch (Message) {
    case WM_SIZE:
        // smth
        break;
    case WM_DESTROY:
        break;
    case WM_CLOSE:
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

        PatBlt(DeviceContext, X, Y, Width, Height, BLACKNESS);
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
    WindowClass.lpfnWndProc = MainWindowCallback;
    WindowClass.hInstance = hInstance;
    /*WindowClass.icon;*/
    WindowClass.lpszClassName = "HandmadeHeroWindowClass";

    if (RegisterClass(&WindowClass)) {
        HWND hWindow = CreateWindowEx(
            0, WindowClass.lpszClassName, "Handmade Hero",
            WS_OVERLAPPEDWINDOW | WS_VISIBLE, CW_USEDEFAULT, CW_USEDEFAULT,
            CW_USEDEFAULT, CW_USEDEFAULT, 0, 0, hInstance, 0);

        if (hWindow) {
            for (;;) {
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
