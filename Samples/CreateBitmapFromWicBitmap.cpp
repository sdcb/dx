// This sample demonstrates how to load an embedded resource (created with 
// MakeResource.cpp) as a bitmap so that it can be rendered with Direct2D.
// This technique works perfectly well with desktop, Windows Store,
// and Windows Phone apps.

#ifndef UNICODE
#define UNICODE
#endif

#include "..\dx.h"

#pragma comment(lib, "user32.lib")

using namespace KennyKerr;

Color const COLOR_WHITE(1.0f, 1.0f, 1.0f);

static Direct2D::Factory factory;
static Direct2D::HwndRenderTarget target;
static Direct2D::Bitmap bitmap;
static Wic::FormatConverter image;

unsigned char const * MakeResourceBuffer();
unsigned MakeResourceSize();

static void CreateDeviceIndependentResources()
{
    auto factory = Wic::CreateFactory();
    auto stream = factory.CreateStream();

    stream.InitializeFromMemory(const_cast<BYTE *>(MakeResourceBuffer()),
                                MakeResourceSize());

    auto decoder = factory.CreateDecoderFromStream(stream);
    auto source = decoder.GetFrame();

    image = factory.CreateFormatConverter();
    image.Initialize(source);
}

static void CreateDeviceResources()
{
    bitmap = target.CreateBitmapFromWicBitmap(image);
}

static void Draw()
{
    target.Clear(COLOR_WHITE);
    target.DrawBitmap(bitmap);
}

int __stdcall wWinMain(HINSTANCE module, HINSTANCE, PWSTR, int)
{
    ComInitialize com;

    factory = Direct2D::CreateFactory();
    CreateDeviceIndependentResources();

    WNDCLASS wc = {};
    wc.hCursor = LoadCursor(nullptr, IDC_ARROW);
    wc.hInstance = module;
    wc.lpszClassName = L"window";
    wc.style = CS_HREDRAW | CS_VREDRAW;

    wc.lpfnWndProc = [] (HWND window, UINT message, WPARAM wparam, LPARAM lparam) -> LRESULT
    {
        if (WM_PAINT == message)
        {
            PAINTSTRUCT ps;
            VERIFY(BeginPaint(window, &ps));

            if (!target)
            {
                target = factory.CreateHwndRenderTarget(window);
                CreateDeviceResources();
            }

            target.BeginDraw();

            Draw();

            if (D2DERR_RECREATE_TARGET == target.EndDraw())
            {
                target.Reset();
            }

            EndPaint(window, &ps);
            return 0;
        }

        if (WM_SIZE == message)
        {
            if (target && S_OK != target.Resize(SizeU(LOWORD(lparam), HIWORD(lparam))))
            {
                target.Reset();
            }
        }

        if (WM_DESTROY == message)
        {
            PostQuitMessage(0);
            return 0;
        }

        return DefWindowProc(window, message, wparam, lparam);
    };

    RegisterClass(&wc);

    CreateWindow(wc.lpszClassName, L"Direct2D", WS_OVERLAPPEDWINDOW | WS_VISIBLE,
                  CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT,
                  nullptr, nullptr, module, nullptr);

    MSG message;
    BOOL result;

    while (result = GetMessage(&message, 0, 0, 0))
    {
        if (-1 != result) DispatchMessage(&message);
    }

    image.Reset();
    bitmap.Reset();
    target.Reset();
    factory.Reset();
}
