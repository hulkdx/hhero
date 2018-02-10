#include <windows.h>
#include <stdint.h>

struct Win32_offscreen_buffer
{
    BITMAPINFO info;
    void      *memory;
    int        width;
    int        height;
    int        pitch;
    int        bytePerPixel;
};

static bool                   running;
static Win32_offscreen_buffer globalBackBuffer;

static void renderWeirdGradient(Win32_offscreen_buffer buffer, int xOffset, int yOffset)
{
    
    uint8_t *row = (uint8_t *) buffer.memory;
    for (int y = 0; y < buffer.height; ++y)
    {
        uint32_t *pixel = (uint32_t *)row;
        for (int x = 0; x < buffer.width; ++x)
        {
            /*
            Pixel in memory: BB GG RR xx
            */
            uint8_t blue = (x + xOffset);
            uint8_t green = (y + yOffset);
            // uint8_t red = (X + xOffset);
            *pixel++ = (blue | (green << 8));
        }
        row += buffer.pitch;
    }
}

static void Win32ResizeDIBSection(Win32_offscreen_buffer *buffer, int width, int height)
{
    if (buffer->memory)
    {
        VirtualFree(buffer->memory, 0, MEM_RELEASE);
    }

    buffer->width = width;
    buffer->height = height;

    buffer->info.bmiHeader.biSize           = sizeof(buffer->info.bmiHeader);
    buffer->info.bmiHeader.biWidth          = buffer->width;
    buffer->info.bmiHeader.biHeight         = -buffer->height;
    buffer->info.bmiHeader.biPlanes         = 1;
    buffer->info.bmiHeader.biBitCount       = 32;
    buffer->info.bmiHeader.biCompression    = BI_RGB;
    buffer->info.bmiHeader.biSizeImage      = 0;
    buffer->info.bmiHeader.biXPelsPerMeter  = 0;
    buffer->info.bmiHeader.biYPelsPerMeter  = 0;
    buffer->info.bmiHeader.biClrUsed        = 0;
    buffer->info.bmiHeader.biClrImportant   = 0;

    int bitmapMemorySize = buffer->bytePerPixel * buffer->width * buffer->height;
    buffer->memory = VirtualAlloc(0, bitmapMemorySize, MEM_COMMIT, PAGE_READWRITE);
    buffer->pitch = width * buffer->bytePerPixel;

}

static void Win32DisplayBufferInWindow(HDC deviceContext, RECT windowRect,
                                       Win32_offscreen_buffer buffer,
                                       int x, int y, int width, int height)
{
    int windowWidth  = windowRect.right  - windowRect.left;
    int windowHeight = windowRect.bottom - windowRect.top;

    StretchDIBits(deviceContext,
                0, 0, buffer.width, buffer.height,
                0, 0, windowWidth, windowHeight,
                buffer.memory,
                &buffer.info,
                DIB_RGB_COLORS, SRCCOPY);
}

LRESULT CALLBACK
Win32MainWindowCallback(HWND   window,
                        UINT   message,
                        WPARAM wParam,
                        LPARAM lParam)
{
    LRESULT result = 0;

    switch (message)
    {
        case WM_SIZE:
        {
            RECT clientRect;
            GetClientRect(window, &clientRect);
            int height = clientRect.bottom - clientRect.top;
            int width  = clientRect.right - clientRect.left;
            Win32ResizeDIBSection(&globalBackBuffer, width, height);
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

            Win32DisplayBufferInWindow(deviceContext, clientRect, globalBackBuffer, x, y, width, height);
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
        HWND window = CreateWindowEx(0,
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
        if (window)
        {
            running = true;
            int xOffset = 0;
            while (running)
            {
                MSG message;
                while (PeekMessage(&message, 0, 0, 0, PM_REMOVE))
                {
                    if (message.message == WM_QUIT)
                    {
                        running = false;
                    }
                    TranslateMessage(&message);
                    DispatchMessage(&message);
                }
                renderWeirdGradient(globalBackBuffer, xOffset, 0);
                xOffset++;

                HDC deviceContext = GetDC(window);
                RECT clientRect;
                GetClientRect(window, &clientRect);
                int windowWidth = clientRect.right - clientRect.left;
                int windowHeight = clientRect.bottom - clientRect.top;
                Win32DisplayBufferInWindow(deviceContext, clientRect,
                    globalBackBuffer, 0, 0, windowWidth, windowHeight);
                ReleaseDC(window, deviceContext);

            }
        }
    }

    return(0);
}
