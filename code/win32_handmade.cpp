#include <windows.h>
#include <stdint.h>

static bool       running;
static BITMAPINFO bitmapInfo;
static void       *bitmapMemory;
static int        bitmapWidth;
static int        bitmapHeight;

static void Win32ResizeDIBSection(int width, int height)
{
  if (bitmapMemory)
  {
    VirtualFree(bitmapMemory, 0, MEM_RELEASE);
  }

  bitmapWidth = width;
  bitmapHeight = height;

  bitmapInfo.bmiHeader.biSize           = sizeof(bitmapInfo.bmiHeader);
  bitmapInfo.bmiHeader.biWidth          = width;
  bitmapInfo.bmiHeader.biHeight         = -height;
  bitmapInfo.bmiHeader.biPlanes         = 1;
  bitmapInfo.bmiHeader.biBitCount       = 32;
  bitmapInfo.bmiHeader.biCompression    = BI_RGB;
  bitmapInfo.bmiHeader.biSizeImage      = 0;
  bitmapInfo.bmiHeader.biXPelsPerMeter  = 0;
  bitmapInfo.bmiHeader.biYPelsPerMeter  = 0;
  bitmapInfo.bmiHeader.biClrUsed        = 0;
  bitmapInfo.bmiHeader.biClrImportant   = 0;

  int bytePerPixel = 4;
  int bitmapMemorySize = bytePerPixel * width * height;
  bitmapMemory = VirtualAlloc(0, bitmapMemorySize, MEM_COMMIT, PAGE_READWRITE);

  int pitch = width * bytePerPixel;
  uint8_t *row = (uint8_t *) bitmapMemory;

  for (int y = 0; y < bitmapHeight; ++y)
  {
    uint8_t *pixel = (uint8_t *)row;
    for (int x = 0; x < bitmapWidth; ++x)
    {
      /*
        Pixel in memory: BB GG RR xx
      */
      *pixel = 225;
      ++pixel;

      *pixel = 0;
      ++pixel;

      *pixel = 0;
      ++pixel;

      *pixel = 0;
      ++pixel;
    }
    row += pitch;
  }
}

static void win32UpdateWindow(HDC deviceContext, RECT *windowRect,
                              int x, int y, int width, int height)
{
  int windowWidth = windowRect->right - windowRect->left;
  int windowHeight = windowRect->bottom - windowRect->top;

  StretchDIBits(deviceContext,
                0, 0, bitmapWidth, bitmapHeight,
                0, 0, windowWidth, windowHeight,
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

      RECT clientRect;
      GetClientRect(window, &clientRect);

      win32UpdateWindow(deviceContext, &clientRect, x, y, width, height);
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
