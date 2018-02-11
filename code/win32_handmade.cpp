#include <windows.h>
#include <stdint.h>
#include <xinput.h>

struct Win32_offscreen_buffer
{
    BITMAPINFO info;
    void      *memory;
    int        width;
    int        height;
    int        pitch;
    int        bytePerPixel;
};

struct Win32_window_dimension
{
    int width;
    int height;
};

DWORD WINAPI XInputGetState(DWORD dwUserIndex, XINPUT_STATE* pState);
DWORD WINAPI XInputSetState(DWORD dwUserIndex, XINPUT_VIBRATION* pVibration);

static bool                   globalRunning;
static Win32_offscreen_buffer globalBackBuffer;


Win32_window_dimension Win32GetWindowDimension (HWND window)
{
    Win32_window_dimension result;

    RECT clientRect;
    GetClientRect(window, &clientRect);
    result.height = clientRect.bottom - clientRect.top;
    result.width  = clientRect.right - clientRect.left;

    return result;
}

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
    buffer->bytePerPixel = 4;

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

static void Win32DisplayBufferInWindow(HDC deviceContext,
                                       int windowWidth, int windowHeight,
                                       Win32_offscreen_buffer buffer,
                                       int x, int y, int width, int height)
{
    // TODO aspect ratio
    StretchDIBits(deviceContext,
                0, 0, windowWidth, windowHeight,
                0, 0, buffer.width, buffer.height,
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
        } break;
        case WM_DESTROY:
        {
            OutputDebugStringA("WM_DESTROY\n");
        } break;
        case WM_CLOSE:
        {
            globalRunning = false;
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

            Win32_window_dimension dimension = Win32GetWindowDimension(window);

            Win32DisplayBufferInWindow(deviceContext, dimension.width, dimension.height,
                globalBackBuffer, x, y, width, height);
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

    Win32ResizeDIBSection(&globalBackBuffer, 1280, 720);

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
            globalRunning = true;
            int xOffset = 0;
            while (globalRunning)
            {
                MSG message;
                while (PeekMessage(&message, 0, 0, 0, PM_REMOVE))
                {
                    if (message.message == WM_QUIT)
                    {
                        globalRunning = false;
                    }
                    TranslateMessage(&message);
                    DispatchMessage(&message);
                }

                for (DWORD controllerIndex=0; controllerIndex< XUSER_MAX_COUNT; controllerIndex++ )
                {
                    XINPUT_STATE controllerState;
                    if (XInputGetState(controllerIndex, &controllerState) == ERROR_SUCCESS)
                    {
                        // Controller is plugged in
                        XINPUT_GAMEPAD *pad = &controllerState.Gamepad;

                        bool up = (pad->wButtons & XINPUT_GAMEPAD_DPAD_UP);
                        bool down = (pad->wButtons & XINPUT_GAMEPAD_DPAD_DOWN);
                        bool left = (pad->wButtons & XINPUT_GAMEPAD_DPAD_LEFT);
                        bool right = (pad->wButtons & XINPUT_GAMEPAD_DPAD_RIGHT);
                        bool start = (pad->wButtons & XINPUT_GAMEPAD_START);
                        bool back = (pad->wButtons & XINPUT_GAMEPAD_BACK);
                        bool leftShoulder = (pad->wButtons & XINPUT_GAMEPAD_LEFT_SHOULDER);
                        bool rightShoulder = (pad->wButtons & XINPUT_GAMEPAD_RIGHT_SHOULDER);
                        bool aButton = (pad->wButtons & XINPUT_GAMEPAD_A);
                        bool bButton = (pad->wButtons & XINPUT_GAMEPAD_B);
                        bool xButton = (pad->wButtons & XINPUT_GAMEPAD_X);
                        bool yButton = (pad->wButtons & XINPUT_GAMEPAD_Y);

                        uint16_t stickX = pad->sThumbLX;
                        uint16_t stickY = pad->sThumbLY;
                    }
                    else
                    {
                        // The controller is not available.
                    }
                }

                renderWeirdGradient(globalBackBuffer, xOffset, 0);
                xOffset++;

                HDC deviceContext = GetDC(window);
                Win32_window_dimension dimension = Win32GetWindowDimension(window);
                Win32DisplayBufferInWindow(deviceContext,
                                           dimension.width, dimension.height,
                                           globalBackBuffer, 0, 0,
                                           dimension.width, dimension.height);
                ReleaseDC(window, deviceContext);

            }
        }
    }

    return(0);
}
