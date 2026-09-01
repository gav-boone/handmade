#include <windows.h>

int CALLBACK WinMain(HINSTANCE hInstance, HINSTANCE hPreviousInstance,
                     LPSTR lpCmdLine, int CmdShow) {

    MessageBox(0, "This is handmade.", "handmade", MB_OK | MB_ICONINFORMATION);

    return 0;
}
