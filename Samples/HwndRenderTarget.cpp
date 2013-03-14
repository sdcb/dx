#include "..\dx.h"

// Just for fun, here's an entire Direct2D-based desktop application implemented entirely in WinMain.

using namespace KennyKerr;

Direct2D::Color const COLOR_WHITE(1.0f, 1.0f, 1.0f);
Direct2D::Color const COLOR_BLUE(0.26f, 0.56f, 0.87f);

static Direct2D::Factory factory;
static Direct2D::HwndRenderTarget target;
static Direct2D::SolidColorBrush brush;

int __stdcall wWinMain(HINSTANCE module, HINSTANCE, PWSTR, int)
{
    factory = Direct2D::CreateFactory();

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
                brush = target.CreateSolidColorBrush(COLOR_BLUE);
            }

            target.BeginDraw();
            target.Clear(COLOR_WHITE);
            auto size = target.GetSize();            

            target.DrawRectangle(RectF(100.0f, 100.0f, size.Width - 100.0f, size.Height - 100.0f),
                                 brush,
                                 50.0f);

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

    brush.Reset();
    target.Reset();
    factory.Reset();
}
