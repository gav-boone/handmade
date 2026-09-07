#include <windows.h>
#include <wingdi.h>

#define private_func static
#define local_persist static
#define global_var static

global_var bool RUNNING;

private_func void Win32ResizeDIBSection(int Width, int Height){

    CreateDIBSection(HDC hdc, const BITMAPINFO *lpbmi, UINT usage,
                     void **ppvBits, HANDLE hSection, DWORD offset)

}

private_func
    void Win32UpdateWindow(HWND hWindow, int X, int Y, int Width, int Height){
        StretchDIBits(HDC hdc, int xDest, int yDest, int DestWidth,
                      int DestHeight, int xSrc, int ySrc, int SrcWidth,
                      int SrcHeight, const void *lpBits,
                      const BITMAPINFO *lpbmi, UINT iUsage, DWORD rop)}

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
        RUNNING = false;
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

        Win32UpdateWindow(hWindow, X, Y, Width, Height);

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
            RUNNING = true;
            while (RUNNING) {
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
