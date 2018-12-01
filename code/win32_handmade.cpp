#include <windows.h>
#include <stdint.h>
#include <xinput.h>
#include <dsound.h>

// TODO: Impliment our own sinf()
#include <math.h>

#define PI32 3.14159265359f

typedef int8_t int8;
typedef int16_t int16;
typedef int32_t int32;
typedef int64_t int64;
typedef int32 bool32;

typedef uint8_t uint8;
typedef uint16_t uint16;
typedef uint32_t uint32;
typedef uint64_t uint64;

typedef float  real32;
typedef double real64;

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

struct win32_sound_output
{
	int samplesPerSecond;
	int toneHz;
	uint16 toneVolume;
	uint32 runningSampleIndex;
	int squareWaveCounter;
	int wavePeriod;
	int bytesPerSample;
	int secondaryBufferSize;
	real32 tSine;
	int latencySampleCount;
};

// Note: this is static for now
static bool                   globalRunning;
static Win32_offscreen_buffer globalBackBuffer;
static LPDIRECTSOUNDBUFFER globalSecondaryBuffer;

// Define XInputGetState
#define X_INPUT_GET_STATE(name) DWORD WINAPI name(DWORD dwUserIndex, XINPUT_STATE* pState)
typedef X_INPUT_GET_STATE(x_input_get_state);
X_INPUT_GET_STATE(XInputGetStateStub)
{
    return ERROR_DEVICE_NOT_CONNECTED;
}
static x_input_get_state *XInputGetState_ = XInputGetStateStub;
#define XInputGetState XInputGetState_
// Define XInputSetState
#define X_INPUT_SET_STATE(name) DWORD WINAPI name(DWORD dwUserIndex, XINPUT_VIBRATION* pVibration)
typedef X_INPUT_SET_STATE(x_input_set_state);
X_INPUT_SET_STATE(XInputSetStateStub)
{
    return ERROR_DEVICE_NOT_CONNECTED;
}
static x_input_set_state *XInputSetState_ = XInputSetStateStub;
#define XInputSetState XInputSetState_

#define DIRECT_SOUND_CREATE(name) HRESULT WINAPI name(LPCGUID pcGuidDevice, LPDIRECTSOUND *ppDS, LPUNKNOWN pUnkOuter)
typedef DIRECT_SOUND_CREATE(direct_sound_create);


static void win32LoadXInput()
{
    HMODULE xInputLibrary = LoadLibraryA("xinput1_3.dll"); // on more machine
    if (!xInputLibrary)
    {
        // TODO: diagnostic
        xInputLibrary = LoadLibraryA("xinput1_4.dll");
    }
    if (xInputLibrary)
    {
        XInputGetState = (x_input_get_state*) GetProcAddress(xInputLibrary, "XInputGetState");
        XInputSetState = (x_input_set_state*) GetProcAddress(xInputLibrary, "XInputSetState");
    }
}


static void win32InitDSound(HWND window, int32 samplesPerSec, int32 bufferSize)
{
    // Load the libary:
    HMODULE dSoundLibrary = LoadLibraryA("dsound.dll");

    if (dSoundLibrary) {
        // Get a DirectSound object
        direct_sound_create *directSoundCreate = (direct_sound_create *) GetProcAddress(dSoundLibrary, "DirectSoundCreate");

        LPDIRECTSOUND directSound;
        if (directSoundCreate && SUCCEEDED(directSoundCreate(0, &directSound, 0)))
        {
            WAVEFORMATEX waveFormat    = {};
            waveFormat.wFormatTag      = WAVE_FORMAT_PCM;
            waveFormat.nChannels       = 2;
            waveFormat.nSamplesPerSec  = samplesPerSec;
            waveFormat.wBitsPerSample  = 16;
            waveFormat.nBlockAlign     = waveFormat.nChannels * waveFormat.wBitsPerSample / 8;
            waveFormat.nAvgBytesPerSec = waveFormat.nSamplesPerSec * waveFormat.nBlockAlign;
            waveFormat.cbSize          = 0;
            // cooprativemode
            if (SUCCEEDED(directSound->SetCooperativeLevel(window, DSSCL_PRIORITY)))
            {
                // create primary buffer
                DSBUFFERDESC bufferDescription = {};
                bufferDescription.dwSize  = sizeof(bufferDescription);
                bufferDescription.dwFlags = DSBCAPS_PRIMARYBUFFER;

                LPDIRECTSOUNDBUFFER primaryBuffer;
                if (SUCCEEDED(directSound->CreateSoundBuffer(&bufferDescription, &primaryBuffer, 0)))
                {
                    if (SUCCEEDED(primaryBuffer->SetFormat(&waveFormat)))
                    {
                        //Note: set the format of primary buffer
                        OutputDebugStringA("primary buffer created successfully");
                    }
                }

            }

            // create secondary buffer
            DSBUFFERDESC bufferDescription = {};
            bufferDescription.dwSize  = sizeof(bufferDescription);
            bufferDescription.dwFlags = 0;
            bufferDescription.dwBufferBytes = bufferSize;
            bufferDescription.lpwfxFormat = &waveFormat;


            if (SUCCEEDED(directSound->CreateSoundBuffer(&bufferDescription, &globalSecondaryBuffer, 0)))
            {
              OutputDebugStringA("secondary buffer created successfully");
            }
        }
    }
}

static void
win32FillSoundBuffer(win32_sound_output *SoundOutput, DWORD BytesToLock, DWORD BytesToWrite) {
    // int16  int16  ...
    // [LEFT  RIGHT] LEFT RIGHT LEFT RIGHT ...
    //
    // TODO(sky): More strenuous test!
    // TODO(sky): Switch to a sine wave
    VOID  *Region1;
    DWORD Region1Size;
    VOID  *Region2;
    DWORD Region2Size;

    if (SUCCEEDED(globalSecondaryBuffer->Lock(BytesToLock, BytesToWrite,
                                              &Region1, &Region1Size,
                                              &Region2, &Region2Size,
                                              0)))
    {
        // TODO(sky): assert that Region1Size/Region2Size is valid

        // TODO(sky): Collapse these two loops

        DWORD Region1SampleCount = Region1Size / SoundOutput->bytesPerSample;
        int16_t *SampleOut = (int16_t *)Region1;
        for(DWORD SampleIndex = 0;
            SampleIndex < Region1SampleCount;
            ++SampleIndex)
        {
            // TOOD(sky): Draw this out for people
            real32 SineValue = sinf(SoundOutput->tSine);
            int16_t SampleValue = (int16_t)(SineValue * SoundOutput->toneVolume);
            *SampleOut++ = SampleValue;
            *SampleOut++ = SampleValue;

            SoundOutput->tSine += 2.0f*PI32*1.0f/(real32)SoundOutput->wavePeriod;
            ++SoundOutput->runningSampleIndex;
        }

        DWORD Region2SampleCount = Region2Size / SoundOutput->bytesPerSample;
        SampleOut = (int16_t *)Region2;
        for(DWORD SampleIndex = 0;
            SampleIndex < Region2SampleCount;
            ++SampleIndex)
        {
            real32 SineValue = sinf(SoundOutput->tSine);
            int16_t SampleValue = (int16_t)(SineValue * SoundOutput->toneVolume);
            *SampleOut++ = SampleValue;
            *SampleOut++ = SampleValue;

            SoundOutput->tSine += 2.0f*PI32*1.0f/(real32)SoundOutput->wavePeriod;
            ++SoundOutput->runningSampleIndex;
        }
        globalSecondaryBuffer->Unlock(Region1, Region1Size,
                                      Region2, Region2Size);
    }
}

Win32_window_dimension Win32GetWindowDimension (HWND window)
{
    Win32_window_dimension result;
    RECT clientRect;
    GetClientRect(window, &clientRect);
    result.height = clientRect.bottom - clientRect.top;
    result.width  = clientRect.right - clientRect.left;

    return result;
}

static void renderWeirdGradient(Win32_offscreen_buffer *buffer, int xOffset, int yOffset)
{

    uint8_t *row = (uint8_t *) buffer->memory;
    for (int y = 0; y < buffer->height; ++y)
    {
        uint32_t *pixel = (uint32_t *)row;
        for (int x = 0; x < buffer->width; ++x)
        {
            /*
            Pixel in memory: BB GG RR xx
            */
            uint8_t blue = (x + xOffset);
            uint8_t green = (y + yOffset);
            // uint8_t red = (X + xOffset);
            *pixel++ = (blue | (green << 8));
        }
        row += buffer->pitch;
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

static void Win32DisplayBufferInWindow(Win32_offscreen_buffer *buffer,
                                       HDC deviceContext,
                                       int windowWidth, int windowHeight)
{
    // TODO aspect ratio
    StretchDIBits(deviceContext,
                0, 0, windowWidth, windowHeight,
                0, 0, buffer->width, buffer->height,
                buffer->memory,
                &buffer->info,
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

        case WM_SYSKEYDOWN:
        case WM_SYSKEYUP:
        case WM_KEYDOWN:
        case WM_KEYUP:
        {
            uint32_t vkCode = wParam;
            bool wasDown = ((lParam & (1 << 30)) != 0);
            bool isDown  = ((lParam & (1 << 31)) == 0);
            if (isDown != wasDown)
            {
                switch (vkCode)
                {
                    case 'W':
                    {

                    } break;
                    case 'A':
                    {

                    } break;
                    case 'S':
                    {

                    } break;
                    case 'D':
                    {

                    } break;
                    case 'Q':
                    {

                    } break;
                    case 'E':
                    {

                    } break;
                    case VK_UP:
                    {

                    } break;
                    case VK_LEFT:
                    {

                    } break;
                    case VK_DOWN:
                    {

                    } break;
                    case VK_RIGHT:
                    {

                    } break;
                    case VK_ESCAPE:
                    {
                      OutputDebugStringA("Escape:");

                    } break;
                    case VK_SPACE:
                    {

                    } break;
                    // ALT + F4
                    case VK_F4:
                    {
                        bool32 altKeyWasDown = (lParam & (1 << 29));
                        if (altKeyWasDown) {
                            globalRunning = false;
                        }
                    } break;
                }
            }
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

            Win32DisplayBufferInWindow(&globalBackBuffer, deviceContext,
                dimension.width, dimension.height);
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
    win32LoadXInput();
    WNDCLASSA windowClass = {};

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

            HDC deviceContext = GetDC(window);

            // Note: graphical test
            int xOffset = 0;
            int yOffset = 0;

            // Note: sound test
            win32_sound_output SoundOutput = {};

            SoundOutput.samplesPerSecond = 48000;
            SoundOutput.toneHz = 256;
            SoundOutput.toneVolume = 1000;
            SoundOutput.runningSampleIndex = 0;
            SoundOutput.squareWaveCounter = 0;
            SoundOutput.wavePeriod = SoundOutput.samplesPerSecond / SoundOutput.toneHz;
            SoundOutput.bytesPerSample = sizeof(int16) * 2;
            SoundOutput.secondaryBufferSize = SoundOutput.samplesPerSecond * SoundOutput.bytesPerSample;
            SoundOutput.tSine = 0.0f;
            SoundOutput.latencySampleCount = SoundOutput.samplesPerSecond / 15;

            win32InitDSound(window, SoundOutput.samplesPerSecond, SoundOutput.samplesPerSecond*SoundOutput.bytesPerSample);
            win32FillSoundBuffer(&SoundOutput, 0, SoundOutput.latencySampleCount * SoundOutput.bytesPerSample);
            globalSecondaryBuffer->Play(0, 0, DSBPLAY_LOOPING);

            globalRunning = true;
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

                        xOffset += stickX >> 12;
                    }
                    else
                    {
                        // The controller is not available.
                    }
                }
                // ???
                xOffset++;

                renderWeirdGradient(&globalBackBuffer, xOffset, 0);

                // Sound test:
                DWORD playCursor;
                DWORD writeCursor;
                // NOTE: directSound output test:
                if (SUCCEEDED(globalSecondaryBuffer->GetCurrentPosition(&playCursor, &writeCursor)))
                {
                    DWORD bytesToLock = (SoundOutput.runningSampleIndex * SoundOutput.bytesPerSample) % SoundOutput.secondaryBufferSize;
                    DWORD targetCursor = (playCursor +
                                         (SoundOutput.latencySampleCount * SoundOutput.bytesPerSample)) %
                                         SoundOutput.secondaryBufferSize;
                    DWORD bytesToWrite;
                    if (bytesToLock > targetCursor)
                    {
                        bytesToWrite = SoundOutput.secondaryBufferSize - bytesToLock;
                        bytesToWrite += targetCursor;
                    }
                    else
                    {
                        bytesToWrite = targetCursor - bytesToLock;
                    }

                    win32FillSoundBuffer(&SoundOutput, bytesToLock, bytesToWrite);
                }

                Win32_window_dimension dimension = Win32GetWindowDimension(window);
                Win32DisplayBufferInWindow(&globalBackBuffer,
                                           deviceContext,
                                           dimension.width, dimension.height);

            }
        }
    }

    return(0);
}
