#include <windows.h>

LRESULT CALLBACK
MainWindowCallback(HWND   window,
                   UINT   message,
                   WPARAM wParam,
                   LPARAM lParam)
{
  LRESULT result = 0;

  switch (message) {
    case WM_SIZE:
    {
      OutputDebugStringA("WM_SIZE\n");
    } break;
    case WM_DESTROY:
    {
      OutputDebugStringA("WM_DESTROY\n");
    } break;
    case WM_CLOSE:
    {
      OutputDebugStringA("WM_CLOSE\n");

    } break;
    case WM_ACTIVATEAPP:
    {
      OutputDebugStringA("WM_ACTIVATEAPP\n");

    } break;

    default:
    {
      result = DefWindowProc(window, message, wParam, lParam);
    } break;
  }

  return result;
}

int CALLBACK
WinMain(HINSTANCE instance,
        HINSTANCE prevInstance,
        LPSTR     commandLine,
        int       showCode)
{
  WNDCLASS windowClass = {};
  windowClass.style = CS_OWNDC|CS_HREDRAW|CS_VREDRAW;
  windowClass.lpfnWndProc = MainWindowCallback;
  windowClass.hInstance = instance;
  // windowClass.hIcon = ;
  windowClass.lpszClassName = "handmadeHeroWindowClass";

  RegisterClass(&windowClass)

  return(0);
}
