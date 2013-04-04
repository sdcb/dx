#define NOMINMAX
#ifndef UNICODE
#define UNICODE
#endif

#pragma comment(lib, "user32.lib")

#include "..\dx.h"

using namespace KennyKerr;
using namespace KennyKerr::Direct2D;

Color const COLOR_WHITE(1.0f, 1.0f, 1.0f);
Color const COLOR_BLUE(0.26f, 0.56f, 0.87f);

static Factory1 factory;
static DeviceContext target;
static SolidColorBrush brush;
static Dxgi::SwapChain1 swapChain;
static DirectWrite::TextLayout textLayout;

static void CreateDeviceIndependentResources()
{
    using namespace KennyKerr::DirectWrite;

    auto factory = DirectWrite::CreateFactory();

    auto format = factory.CreateTextFormat(L"Candara",
                                           100.0f);

    format.SetTextAlignment(TextAlignment::Center);
    format.SetParagraphAlignment(ParagraphAlignment::Center);

    WCHAR text[] = L"Hello World";

    textLayout = factory.CreateTextLayout(text,
                                          _countof(text) - 1,
                                          format,
                                          100.0f,
                                          100.0f);
}

static void CreateDeviceResources()
{
    brush = target.CreateSolidColorBrush(COLOR_BLUE);
}

static void ReleaseDevice()
{
    target.Reset();
    swapChain.Reset();
    brush.Reset();
}

static void Draw()
{
    target.Clear(COLOR_WHITE);
    auto const size = target.GetSize();

    textLayout.SetMaxWidth(size.Width);
    textLayout.SetMaxHeight(size.Height);

    target.DrawTextLayout(Point2F(),
                          textLayout,
                          brush);
}

static void CreateDeviceSwapChainBitmap(Dxgi::SwapChain const & swapChain,
                                        DeviceContext const & target)
{
    auto props = BitmapProperties1(BitmapOptions::Target | BitmapOptions::CannotDraw,
                                   PixelFormat(Dxgi::Format::B8G8R8A8_UNORM, AlphaMode::Ignore));

    target.SetTarget(target.CreateBitmapFromDxgiSurface(swapChain,
                                                        props));
}

static void Render(HWND window)
{
    if (!target)
    {
        auto device = Direct3D::CreateDevice();
        target = factory.CreateDevice(device).CreateDeviceContext();
        auto dxgi = device.GetDxgiFactory();

        Dxgi::SwapChainDescription1 description;
        description.SwapEffect = Dxgi::SwapEffect::Discard;

        swapChain = dxgi.CreateSwapChainForHwnd(device,
                                                window,
                                                description);

        CreateDeviceSwapChainBitmap(swapChain,
                                    target);

        auto const dpi = factory.GetDesktopDpi();
        target.SetDpi(dpi, dpi);
        CreateDeviceResources();
    }

    target.BeginDraw();
    Draw();
    target.EndDraw();

    auto const hr = swapChain.Present();

    if (S_OK != hr && DXGI_STATUS_OCCLUDED != hr)
    {
        ReleaseDevice();
    }
}

static void ResizeSwapChainBitmap()
{
    target.SetTarget();

    if (S_OK == swapChain.ResizeBuffers())
    {
        CreateDeviceSwapChainBitmap(swapChain,
                                    target);
    }
    else
    {
        ReleaseDevice();
    }
}

int __stdcall wWinMain(HINSTANCE module, HINSTANCE, PWSTR, int)
{
    ComInitialize com;

    factory = CreateFactory();
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
            Render(window);
            EndPaint(window, &ps);
            return 0;
        }

        if (WM_SIZE == message)
        {
            if (target && SIZE_MINIMIZED != wparam)
            {
                ResizeSwapChainBitmap();
                Render(window);
            }
            
            return 0;
        }

        if (WM_DISPLAYCHANGE == message)
        {
            Render(window);
            return 0;
        }

        if (WM_DESTROY == message)
        {
            PostQuitMessage(0);
            return 0;
        }

        return DefWindowProc(window, message, wparam, lparam);
    };

    RegisterClass(&wc);

    CreateWindow(wc.lpszClassName, L"dx.codeplex.com", WS_OVERLAPPEDWINDOW | WS_VISIBLE,
                  CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT,
                  nullptr, nullptr, module, nullptr);

    MSG message;
    BOOL result;

    while (result = GetMessage(&message, 0, 0, 0))
    {
        if (-1 != result) DispatchMessage(&message);
    }

    ReleaseDevice();
    factory.Reset();
}
