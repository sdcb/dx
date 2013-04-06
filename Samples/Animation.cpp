// This minimal animation sample illustrates how to use the animation manager to
// schedule a transition to animate a variable's value. This sample uses
// continuous rendering (typically 60fps).

#define NOMINMAX
#ifndef UNICODE
#define UNICODE
#endif

#pragma comment(lib, "user32.lib")

#include "..\dx.h"

using namespace KennyKerr;
using namespace KennyKerr::Direct2D;
using namespace KennyKerr::DirectWrite;

Color const COLOR_WHITE(1.0f, 1.0f, 1.0f);
Color const COLOR_BLUE(0.26f, 0.56f, 0.87f);

static Direct2D::Factory1 factory;
static Direct2D::DeviceContext target;
static Dxgi::SwapChain1 swapChain;
static Direct2D::SolidColorBrush brush;
static DirectWrite::TextFormat textFormat;
static Wam::Manager manager;
static Wam::Variable variable;
static Wam::SimpleTimer timer;

static void CreateDeviceIndependentResources()
{
    auto factory = DirectWrite::CreateFactory();

    textFormat = factory.CreateTextFormat(L"Consolas",
                                          140.0f);

    textFormat.SetTextAlignment(TextAlignment::Center);
    textFormat.SetParagraphAlignment(ParagraphAlignment::Center);

    manager = Wam::CreateManager();
    auto library = Wam::CreateTransitionLibrary();

    auto transition = library.CreateAccelerateDecelerateTransition(
        10.0,
        100.0,
        0.2,
        0.8);

    variable = manager.CreateAnimationVariable(0.0);

    manager.ScheduleTransition(variable,
                               transition,
                               timer.GetTime());
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

    manager.Update(timer.GetTime());
    auto value = variable.GetValue();

    WCHAR text[10];
    auto length = swprintf_s(text, L"%.1f", value);

    target.DrawText(text,
                    length,
                    textFormat,
                    RectF(0.0f, 0.0f, size.Width, size.Height),
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

    auto window = CreateWindow(wc.lpszClassName, L"dx.codeplex.com", WS_OVERLAPPEDWINDOW | WS_VISIBLE,
                               CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT,
                               nullptr, nullptr, module, nullptr);
    ASSERT(window);

    MSG message;

    for (;;)
    {
        Render(window);

        while (PeekMessage(&message,
                           nullptr,
                           0, 0,
                           PM_REMOVE))
        {
            DispatchMessage(&message);
        }

        if (WM_QUIT == message.message)
        {
            break;
        }
    }

    ReleaseDevice();

    variable.Reset();
    manager.Reset();
    textFormat.Reset();
    factory.Reset();
}
