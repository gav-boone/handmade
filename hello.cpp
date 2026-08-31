#include <windows.h>

struct Test {
  bool isProne;
  char velocity;
  int x;
  int y;
};

int CALLBACK WinMain(HINSTANCE hInstance, HINSTANCE hPreviousInstance,
                     LPSTR lpCmdLine, int CmdShow) {
  Test test;
  test.x = 10;
  test.y = 10;
  test.isProne = 0;
  test.velocity = 100;

  for (int i = 0; i < 10; i++) {
    test.velocity += i * 10;
  }

  return 0;
}
