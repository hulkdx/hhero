#include <windows.h>

static bool       running;
static BITMAPINFO bitmapInfo;
static void       *bitmapMemory;
static HBITMAP    bitmapHandle;
static HDC        bitmapDeviceContext;

static void Win32ResizeDIBSection(int width, int height)
{
  // Free DIBSection
  if (bitmapHandle)
  {
    DeleteObject(bitmapHandle);
  }
  if (!bitmapDeviceContext) {
    bitmapDeviceContext = CreateCompatibleDC(0);
  }

  bitmapInfo.bmiHeader.biSize           = sizeof(bitmapInfo.bmiHeader);
  bitmapInfo.bmiHeader.biWidth          = width;
  bitmapInfo.bmiHeader.biHeight         = height;
  bitmapInfo.bmiHeader.biPlanes         = 1;
  bitmapInfo.bmiHeader.biBitCount       = 32;
  bitmapInfo.bmiHeader.biCompression    = BI_RGB;
  bitmapInfo.bmiHeader.biSizeImage      = 0;
  bitmapInfo.bmiHeader.biXPelsPerMeter  = 0;
  bitmapInfo.bmiHeader.biYPelsPerMeter  = 0;
  bitmapInfo.bmiHeader.biClrUsed        = 0;
  bitmapInfo.bmiHeader.biClrImportant   = 0;

  bitmapHandle = CreateDIBSection(bitmapDeviceContext, &bitmapInfo,
                                  DIB_RGB_COLORS,
                                  &bitmapMemory, 0, 0);
}

static void win32UpdateWindow(HDC deviceContext,
                              int x, int y, int width, int height)
{
  StretchDIBits(deviceContext,
                x, y, width, height,
                x, y, width, height,
                bitmapMemory,
                &bitmapInfo,
                DIB_RGB_COLORS, SRCCOPY);
}

LRESULT CALLBACK
Win32MainWindowCallback(HWND   window,
                        UINT   message,
                        WPARAM wParam,
                        LPARAM lParam)
{
  LRESULT result = 0;

  switch (message) {
    case WM_SIZE:
    {
      RECT clientRect;
      GetClientRect(window, &clientRect);
      int height = clientRect.bottom - clientRect.top;
      int width  = clientRect.right - clientRect.left;
      Win32ResizeDIBSection(width, height);
      OutputDebugStringA("WM_SIZE\n");
    } break;
    case WM_DESTROY:
    {
      OutputDebugStringA("WM_DESTROY\n");
    } break;
    case WM_CLOSE:
    {
      running = false;
      OutputDebugStringA("WM_CLOSE\n");

    } break;
    case WM_ACTIVATEAPP:
    {
      OutputDebugStringA("WM_ACTIVATEAPP\n");

    } break;
    case WM_PAINT:
    {
      PAINTSTRUCT paint;
      HDC deviceContext = BeginPaint(window, &paint);
      int x      = paint.rcPaint.left;
      int y      = paint.rcPaint.top;
      int height = paint.rcPaint.bottom - paint.rcPaint.top;
      int width  = paint.rcPaint.right - paint.rcPaint.left;
      win32UpdateWindow(deviceContext, x, y, width, height);
      EndPaint(window, &paint);
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
  windowClass.lpfnWndProc = Win32MainWindowCallback;
  windowClass.hInstance = instance;
  // windowClass.hIcon = ;
  windowClass.lpszClassName = "handmadeHeroWindowClass";

  if (RegisterClass(&windowClass))
  {
    HWND windowHandle = CreateWindowEx(0,
                                       windowClass.lpszClassName,
                                       "Handmade Hero",
                                       WS_OVERLAPPEDWINDOW|WS_VISIBLE,
                                       CW_USEDEFAULT,
                                       CW_USEDEFAULT,
                                       CW_USEDEFAULT,
                                       CW_USEDEFAULT,
                                       0,
                                       0,
                                       instance,
                                       0);
    if (windowHandle)
    {
      MSG message;
      running = true;
      while (running)
      {
        BOOL messageResult = GetMessage(&message, 0, 0, 0);
        if (messageResult > 0)
        {
          TranslateMessage(&message);
          DispatchMessage(&message);
        }
        else
        {
          break;
        }
      }
    }
  }

  return(0);
}
