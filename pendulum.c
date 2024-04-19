#include <stdio.h>
#include <stdint.h>
#include <assert.h>

#define _USE_MATH_DEFINES
#include <math.h>

#include <windows.h>

typedef struct Dimension {
    int32_t Width;
    int32_t Height;
} Dimension;

LRESULT CALLBACK WinProcedure(HWND HWnd, UINT UMsg, WPARAM WParam, LPARAM LParam);
void CalcEndPoints(float *EndX, float *EndY, float BeginX, float BeginY, float Length, float Angle);
void DilationMask(void *Display, int32_t X, int32_t Y, uint32_t Color, Dimension DisplayDimension);
void ClearScreen(void *Display, uint32_t Color, Dimension DisplayDimension);
void DrawLine(void *Display, int32_t BeginX, int32_t BeginY, int32_t EndX, int32_t EndY, uint32_t Color, Dimension DisplayDimension);
void DrawCircle(void *Display, int32_t X, int32_t Y, int32_t Radius, uint32_t Color, Dimension DisplayDimension);

int main(void)
{
    HANDLE HInstance = GetModuleHandleW(NULL);

    WNDCLASSW WindowClass = {0};
    WindowClass.lpszClassName = L"ClassPendulum";
    WindowClass.hbrBackground = (HBRUSH) COLOR_WINDOW;
    WindowClass.hCursor = LoadCursor(NULL, IDC_ARROW);
    WindowClass.hInstance = HInstance;
    WindowClass.lpfnWndProc = WinProcedure;

    if(!RegisterClassW(&WindowClass)) return -1;

    uint32_t Width = 800;
    uint32_t Height = 600;

    RECT WindowRect = { 0 };
    WindowRect.right = Width;
    WindowRect.bottom = Height;
    WindowRect.left = 0;
    WindowRect.top = 0;

    AdjustWindowRect(&WindowRect, WS_OVERLAPPEDWINDOW | WS_VISIBLE, 0);
    HWND Window = CreateWindowW(
        WindowClass.lpszClassName,
        L"Pendulum",
        WS_OVERLAPPEDWINDOW | WS_VISIBLE,
        CW_USEDEFAULT, CW_USEDEFAULT,
        WindowRect.right - WindowRect.left,
        WindowRect.bottom - WindowRect.top,
        NULL, NULL,
        NULL, NULL
    );

    GetWindowRect(Window, &WindowRect);

    uint32_t BitmapWidth = Width;
    uint32_t BitmapHeight = Height;

    uint32_t BytesPerPixel = 4;

    uint32_t BitmapTotalSize = BitmapWidth * BitmapHeight;
    uint32_t DisplayTotalSize = BitmapTotalSize * BytesPerPixel;

    void *Display = malloc(DisplayTotalSize);

    BITMAPINFO BitmapInfo;
    BitmapInfo.bmiHeader.biSize = sizeof(BitmapInfo.bmiHeader);
    BitmapInfo.bmiHeader.biWidth = BitmapWidth;
    BitmapInfo.bmiHeader.biHeight = -BitmapHeight;
    BitmapInfo.bmiHeader.biPlanes = 1;
    BitmapInfo.bmiHeader.biBitCount = 32;
    BitmapInfo.bmiHeader.biCompression = BI_RGB;

    HDC hdc = GetDC(Window);

    MSG msg = { 0 };
    int32_t running = 1;


    float ArmSocketLength = 250;
    float ArmSocketAngle = M_PI / 6;

    float ArmSocketBeginX = Width / 2;
    float ArmSocketBeginY = Height / 2;

    float ArmSocketEndX = 0;
    float ArmSocketEndY = 0;

    float ArmSocketAngleVel = 0;
    float ArmSocketAngleAcc = 0;

    float Gravity = 0.01f;

    Dimension DisplayDimension = {BitmapWidth, BitmapHeight}; 

    while (running) {

        while (PeekMessageW(&msg, NULL, 0, 0, PM_REMOVE)) {
            switch (msg.message) {
                case WM_QUIT: {
                    running = 0;
                    break;
                }
            }
            TranslateMessage(&msg);
            DispatchMessageW(&msg);
        }
        
        float Force = Gravity * sin(ArmSocketAngle);
        ArmSocketAngleAcc = (-1 * Force) / ArmSocketLength;
        ArmSocketAngleVel += ArmSocketAngleAcc;
        ArmSocketAngle += ArmSocketAngleVel;

        CalcEndPoints(&ArmSocketEndX, &ArmSocketEndY, ArmSocketBeginX, ArmSocketBeginY, ArmSocketLength, ArmSocketAngle + M_PI / 2);

        ClearScreen(Display, 0x00000000, DisplayDimension);
        DrawLine(Display, ArmSocketBeginX, ArmSocketBeginY, ArmSocketEndX, ArmSocketEndY, 0x00FFFFFF, DisplayDimension);
        DrawCircle(Display, ArmSocketEndX, ArmSocketEndY, 15, 0x0077FF77, DisplayDimension);
        
        StretchDIBits(
            hdc, 0, 0,
            BitmapWidth, BitmapHeight,
            0, 0,
            BitmapWidth, BitmapHeight,
            Display, &BitmapInfo,
            DIB_RGB_COLORS,
            SRCCOPY
        );
    }

    return 0;
}

LRESULT CALLBACK WinProcedure(HWND HWnd, UINT UMsg, WPARAM WParam, LPARAM LParam)
{
    switch (UMsg) {
        case WM_DESTROY: {
            PostQuitMessage(0);
            break;
        }
        default: {
            return DefWindowProcW(HWnd, UMsg, WParam, LParam);
            break;
        }
    }
    return 0;
}

void CalcEndPoints(float *EndX, float *EndY, float BeginX, float BeginY, float Length, float Angle)
{
    *EndX = BeginX + (Length * cos(Angle));
    *EndY = BeginY + (Length * sin(Angle));
}

void DilationMask(void *Display, int32_t X, int32_t Y, uint32_t Color, Dimension DisplayDimension)
{
    if(X >= 0 && X < DisplayDimension.Width && Y >= 0 && Y < DisplayDimension.Height) {
        ((uint32_t *) Display)[X + Y * DisplayDimension.Width] = Color;
    }
}

void ClearScreen(void *Display, uint32_t Color, Dimension DisplayDimension)
{
    for(int32_t j = 0; j < DisplayDimension.Height; ++j) {
        for(int32_t i = 0; i < DisplayDimension.Width; ++i) {
            DilationMask(Display, i, j, Color, DisplayDimension);
        }
    }
}

#define ABS(x) (x < 0 ? -x : x)

void DrawLine(void *Display, int32_t BeginX, int32_t BeginY, int32_t EndX, int32_t EndY, uint32_t Color, Dimension DisplayDimension)
{
    int32_t DX = EndX - BeginX;
    int32_t DY = EndY - BeginY;

    int32_t Steps = ABS(DX) > ABS(DY) ? ABS(DX) : ABS(DY);

    float XInc = DX / (float) Steps;
    float YInc = DY / (float) Steps;

    float FX = BeginX;
    float FY = BeginY;
    for(int i = 0; i <= Steps; ++i) {
        DilationMask(Display, FX, FY, Color, DisplayDimension);
        FX += XInc;
        FY += YInc;
    }
}

void DrawCircle(void *Display, int32_t X, int32_t Y, int32_t Radius, uint32_t Color, Dimension DisplayDimension)
{
    for(int32_t j = -Radius; j <= Radius; ++j) {
        for(int32_t i = -Radius; i <= Radius; ++i) {
            int32_t DX = -i;
            int32_t DY = -j;
            if(((DX * DX) + (DY * DY)) <= (Radius * Radius)) {
                DilationMask(Display, DX + X, DY + Y, Color, DisplayDimension);
            }
        }   
    }
}