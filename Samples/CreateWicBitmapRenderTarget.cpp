#include "..\dx.h"

using namespace KennyKerr;

Direct2D::Color const COLOR_BLUE(0.26f, 0.56f, 0.87f);
Direct2D::Color const COLOR_WHITE(1.0f,  1.0f,  1.0f);
wchar_t const * const FILENAME = L"C:\\temp\\dx.png";

int __stdcall wWinMain(HINSTANCE, HINSTANCE, PWSTR, int)
{
    // Although DirectX does not require the COM apartment to be initialized,
    // the WIC factory relies on COM activation.

    ComInitialize com;

    // Create the WIC factory and use it to create a bitmap that Direct2D will target.

    auto wicFactory = Wic::CreateFactory();
    auto wicBitmap = wicFactory.CreateBitmap(SizeU(600, 400));

    // Create the Direct2D factory and WIC render target.

    auto factory = Direct2D::CreateFactory();
    auto target = factory.CreateWicBitmapRenderTarget(wicBitmap);

    // Draw a blue ellipse on a transparent background.

    auto brush = target.CreateSolidColorBrush(COLOR_BLUE);
    target.BeginDraw();
    target.Clear();

    target.DrawEllipse(Direct2D::Ellipse(Point2F(300.0f, 200.0f), 100.0f, 100.0f),
                       brush,
                       50.0f);

    HR(target.EndDraw());

    // Create a file stream object.

    auto stream = wicFactory.CreateStream();
    HR(stream.InitializeFromFilename(FILENAME));

    // Create a PNG encoder and prepare a frame based on the WIC bitmap.

    auto encoder = wicFactory.CreateEncoder(GUID_ContainerFormatPng);
    encoder.Initialize(stream);
    auto frame = encoder.CreateNewFrame();
    frame.SetSize(wicBitmap.GetSize());

    GUID format;
    wicBitmap.GetPixelFormat(format);
    frame.SetPixelFormat(format);

    frame.WriteSource(wicBitmap);

    // Commit the formatted results to disk.

    frame.Commit();
    encoder.Commit();

    // Open the resulting file just to make sure it all worked!

    ShellExecute(nullptr, nullptr, FILENAME, nullptr, nullptr, SW_SHOWDEFAULT);
}
