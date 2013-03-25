// This sample illustrates how to create a bitmap brush and fill a shape with it.

#define NOMINMAX
#ifndef UNICODE
#define UNICODE
#endif
#pragma comment(lib, "shell32.lib") 

#include "..\dx.h"
#include <algorithm> // std::min

using namespace KennyKerr;
using namespace KennyKerr::Direct2D;

unsigned char const * MakeResourceBuffer();
unsigned MakeResourceSize();

static Wic::FormatConverter LoadWicBitmap()
{
    auto factory = Wic::CreateFactory();
    auto stream = factory.CreateStream();

    stream.InitializeFromMemory(const_cast<BYTE *>(MakeResourceBuffer()),
                                MakeResourceSize());

    auto decoder = factory.CreateDecoderFromStream(stream);
    auto source = decoder.GetFrame();

    auto image = factory.CreateFormatConverter();
    image.Initialize(source);
    return image;
}

static void Draw(DeviceContext const & target)
{
    target.Clear();
    auto const size = target.GetSize();

    Point2F center(size.Width / 2.0f,
                   size.Height / 2.0f);

    auto radius = std::min(size.Width, size.Height) / 3.0f;

    // First, draw image with 50% transparency

    auto bitmap = target.CreateBitmapFromWicBitmap(LoadWicBitmap());

    target.DrawBitmap(bitmap, 0.5f);

    // Next, fill circle with image

    auto bitmapBrush = target.CreateBitmapBrush(bitmap);

    target.FillEllipse(Direct2D::Ellipse(center, radius, radius),
                       bitmapBrush);

    // Next, draw circle with red brush

    auto solid = target.CreateSolidColorBrush(Color(1.0f));

    target.DrawEllipse(Direct2D::Ellipse(center, radius, radius),
                       solid,
                       10.0f);
}

wchar_t const * const FILENAME = L"C:\\temp\\dx.png";

int __stdcall wWinMain(HINSTANCE, HINSTANCE, PWSTR, int)
{
    ComInitialize com;

    {
        auto driver = Direct3D::CreateDevice();
        auto factory = CreateFactory();
        auto device = factory.CreateDevice(driver);
        auto target = device.CreateDeviceContext();

        BitmapProperties1 props(BitmapOptions::Target,
                                PixelFormat(Dxgi::Format::B8G8R8A8_UNORM, AlphaMode::Premultipled));

        auto bitmap = target.CreateBitmap(SizeU(800, 700), props);
        target.SetTarget(bitmap);

        target.BeginDraw();
        Draw(target);
        HR(target.EndDraw());

        auto wic = Wic::CreateFactory();
        auto stream = wic.CreateStream();
        HR(stream.InitializeFromFilename(FILENAME));

        auto pngEncoder = wic.CreateEncoder(GUID_ContainerFormatPng);
        pngEncoder.Initialize(stream);
        auto frame = pngEncoder.CreateNewFrame();

        auto imageEncoder = wic.CreateImageEncoder(device);

        imageEncoder.WriteFrame(bitmap,
                                frame);

        frame.Commit();
        pngEncoder.Commit();
    }

    // Open the resulting file just to make sure it all worked!

    ShellExecute(nullptr, nullptr, FILENAME, nullptr, nullptr, SW_SHOWDEFAULT);
}
