// This sample demonstrates how to use Direct2D to render a bitmap on the GPU (using a Direct3D device)
// and then encode the resulting bitmap with the Windows Imaging Component (WIC) to produce a PNG file.

// for ShellExecute
#ifndef UNICODE
#define UNICODE
#endif
#pragma comment(lib, "shell32.lib") 

#include "..\dx.h"
using namespace KennyKerr;
using namespace KennyKerr::Direct2D;

Color const COLOR_GREEN(0.45f, 0.56f, 0.1f);
wchar_t const * const FILENAME = L"C:\\temp\\dx.png";

int __stdcall wWinMain(HINSTANCE, HINSTANCE, PWSTR, int)
{
    // Although DirectX does not require the COM apartment to be initialized,
    // the WIC factory relies on COM activation.

    ComInitialize com;

    {
        // Create the Direct3D device driver that represents the physical rendering device (the GPU).

        auto driver = Direct3D::CreateDevice();

        // Create the Direct2D factory, the Direct2D device, and render target.

        auto factory = CreateFactory();
        auto device = factory.CreateDevice(driver);
        auto target = device.CreateDeviceContext();

        // Create the bitmap (on the GPU) that will act as the rendering surface.

        BitmapProperties1 props(BitmapOptions::Target,
                                PixelFormat(Dxgi::Format::B8G8R8A8_UNORM, AlphaMode::Premultiplied));

        auto bitmap = target.CreateBitmap(SizeU(600, 400), props);
        target.SetTarget(bitmap);

        // Draw a green ellipse on a transparent background.

        auto brush = target.CreateSolidColorBrush(COLOR_GREEN);
        target.BeginDraw();
        target.Clear();

        target.DrawEllipse(Direct2D::Ellipse(Point2F(300.0f, 200.0f), 100.0f, 100.0f),
                           brush,
                           50.0f);

        HR(target.EndDraw());

        // Create the WIC factory and a file stream object.

        auto wic = Wic::CreateFactory();
        auto stream = wic.CreateStream();
        HR(stream.InitializeFromFilename(FILENAME));

        // Create a PNG encoder and a new frame to store the image.

        auto pngEncoder = wic.CreateEncoder(GUID_ContainerFormatPng);
        pngEncoder.Initialize(stream);
        auto frame = pngEncoder.CreateNewFrame();

        // Create an image encoder to directly encode the Direct2D image into the frame.

        auto imageEncoder = wic.CreateImageEncoder(device);

        imageEncoder.WriteFrame(bitmap,
                                frame);

        // Commit the formatted results to disk.

        frame.Commit();
        pngEncoder.Commit();
    }

    // Open the resulting file just to make sure it all worked!

    ShellExecute(nullptr, nullptr, FILENAME, nullptr, nullptr, SW_SHOWDEFAULT);
}
