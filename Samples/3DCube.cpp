// This minimal animation sample illustrates how to use the animation manager to
// schedule a transition to animate a variable's value. This sample uses
// continuous rendering (typically 60fps).

#define NOMINMAX
#ifndef UNICODE
#define UNICODE
#endif

#pragma comment(lib, "user32.lib")

#include "..\dx.h"
#include <string>
#include <DirectXMath.h>

using namespace std;
using namespace DirectX;
using namespace KennyKerr;
using namespace KennyKerr::Direct3D;
using namespace KennyKerr::DirectWrite;

Color const COLOR_CORNFLOWERBLUE(0.392156899f, 0.584313750f, 0.929411829f, 1.000000000f);

struct ModelViewProjection
{
    XMFLOAT4X4 model;
    XMFLOAT4X4 view;
    XMFLOAT4X4 projection;
};

// Used to send per-vertex data to the vertex shader.
struct VertexPositionColor
{
    XMFLOAT3 pos;
    XMFLOAT3 color;
};

static const char SimpleVS[] = R"(
// A constant buffer that stores the three basic column-major matrices for composing geometry.
cbuffer ModelViewProjectionConstantBuffer : register(b0)
{
	matrix model;
	matrix view;
	matrix projection;
};

// Per-vertex data used as input to the vertex shader.
struct VertexShaderInput
{
	float3 pos : POSITION;
	float3 color : COLOR0;
};

// Per-pixel color data passed through the pixel shader.
struct PixelShaderInput
{
	float4 pos : SV_POSITION;
	float3 color : COLOR0;
};

// Simple shader to do vertex processing on the GPU.
PixelShaderInput main( VertexShaderInput input )
{
	PixelShaderInput output;
	float4 pos = float4(input.pos, 1.0f);

		// Transform the vertex position into projected space.
		pos = mul( pos, model );
	pos = mul( pos, view );
	pos = mul( pos, projection );
	output.pos = pos;

	// Pass the color through without modification.
	output.color = input.color;

	return output;
}
)";

static char SimplePS[] = R"(
// Per-pixel color data passed through the pixel shader.
struct PixelShaderInput
{
	float4 pos : SV_POSITION;
	float3 color : COLOR0;
};

// A pass-through function for the (interpolated) color data.
float4 main( PixelShaderInput input ) : SV_TARGET
{
	return float4(input.color, 1.0f);
}
)";

static Direct3D::Device device;
static Direct3D::DeviceContext context;
static Direct3D::RenderTargetView renderTargetView;
static Direct3D::DepthStencilView depthStencilView;
static Direct3D::VertexShader vertexShader;
static Direct3D::InputLayout inputLayout;
static Direct3D::PixelShader pixelShader;
static Direct3D::Buffer constantsBuffer;
static Direct3D::Buffer vertexBuffer;
static Direct3D::Buffer indexBuffer;
ModelViewProjection constants;
unsigned indexCount;

static Dxgi::SwapChain1 swapChain;
static Wam::Manager manager;
static Wam::Variable variable;
static Wam::SimpleTimer timer;

static void CreateDeviceIndependentResources()
{
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

static void ReleaseDeviceIndependentResources()
{
    variable.Reset();
    manager.Reset();
}

static string GetCompileTargetSuffix()
{
    switch (device.GetFeatureLevel())
    {
    case FeatureLevel::_9_1:
    case FeatureLevel::_9_2:
        return "_4_0_level_9_1";

    case FeatureLevel::_9_3:
        return "_4_0_level_9_3";

    case FeatureLevel::_10_0:
        return "_4_0";

    case FeatureLevel::_10_1:
        return "_4_1";

    case FeatureLevel::_11_0:
        return "_5_0";
    }

    return "_5_1";
}

static void CreateDeviceResources()
{
    auto targetSuffix = GetCompileTargetSuffix();

    auto vertexShaderCode = Direct3D::Compile(("vs" + targetSuffix).c_str(), SimpleVS, _countof(SimpleVS));
    vertexShader = device.CreateVertexShader(vertexShaderCode.GetBufferPointer(), vertexShaderCode.GetBufferSize());

    const InputElementDescription vertexDesc[] =
    {
        InputElementDescription{ Dxgi::Format::R32G32B32_FLOAT, "POSITION", 0, 0, 0 },
        InputElementDescription{ Dxgi::Format::R32G32B32_FLOAT, "COLOR", 0, 0, 12 },
    };

    inputLayout = device.CreateInputLayout(vertexDesc, _countof(vertexDesc), vertexShaderCode.GetBufferPointer(), vertexShaderCode.GetBufferSize());

    auto pixelShaderCode = Direct3D::Compile(("ps" + targetSuffix).c_str(), SimplePS, _countof(SimplePS));
    pixelShader = device.CreatePixelShader(pixelShaderCode.GetBufferPointer(), pixelShaderCode.GetBufferSize());

    constantsBuffer = device.CreateBuffer(BufferDescription{ sizeof(ModelViewProjection), BindFlag::ConstantBuffer });

    const VertexPositionColor cubeVertices[] =
    {
        { XMFLOAT3(-0.5f, -0.5f, -0.5f), XMFLOAT3(0.0f, 0.0f, 0.0f) },
        { XMFLOAT3(-0.5f, -0.5f, 0.5f), XMFLOAT3(0.0f, 0.0f, 1.0f) },
        { XMFLOAT3(-0.5f, 0.5f, -0.5f), XMFLOAT3(0.0f, 1.0f, 0.0f) },
        { XMFLOAT3(-0.5f, 0.5f, 0.5f), XMFLOAT3(0.0f, 1.0f, 1.0f) },
        { XMFLOAT3(0.5f, -0.5f, -0.5f), XMFLOAT3(1.0f, 0.0f, 0.0f) },
        { XMFLOAT3(0.5f, -0.5f, 0.5f), XMFLOAT3(1.0f, 0.0f, 1.0f) },
        { XMFLOAT3(0.5f, 0.5f, -0.5f), XMFLOAT3(1.0f, 1.0f, 0.0f) },
        { XMFLOAT3(0.5f, 0.5f, 0.5f), XMFLOAT3(1.0f, 1.0f, 1.0f) },
    };

    vertexBuffer = device.CreateBuffer(BufferDescription{ sizeof(cubeVertices), BindFlag::VertexBuffer }, SubresourceData{ &cubeVertices });

    // Load mesh indices. Each trio of indices represents
    // a triangle to be rendered on the screen.
    // For example: 0,2,1 means that the vertices with indexes
    // 0, 2 and 1 from the vertex buffer compose the 
    // first triangle of this mesh.
    const unsigned short cubeIndices[] =
    {
        0, 2, 1, // -x
        1, 2, 3,

        4, 5, 6, // +x
        5, 7, 6,

        0, 1, 5, // -y
        0, 5, 4,

        2, 6, 7, // +y
        2, 7, 3,

        0, 4, 6, // -z
        0, 6, 2,

        1, 3, 7, // +z
        1, 7, 5,
    };

    indexCount = _countof(cubeIndices);
    indexBuffer = device.CreateBuffer(BufferDescription{ sizeof(cubeIndices), BindFlag::IndexBuffer }, SubresourceData{ &cubeIndices });
}

static void ReleaseDevice()
{
    device.Reset();
    context.Reset();
    renderTargetView.Reset();
    swapChain.Reset();
}

static void Draw()
{
    manager.Update(timer.GetTime());
    auto radians = static_cast<float>(variable.GetValue());

    XMStoreFloat4x4(&constants.model, XMMatrixTranspose(XMMatrixRotationY(radians)));

    context.ClearRenderTargetView(renderTargetView, COLOR_CORNFLOWERBLUE);
    context.ClearDepthStencilView(depthStencilView);

    context.UpdateSubresource(constantsBuffer, &constants);

    Buffer vertexBuffers[] ={ vertexBuffer };
    unsigned strides[] ={ sizeof(VertexPositionColor) };
    unsigned offsets[] ={ 0 };
    context.IASetVertexBuffers(0, 1, vertexBuffers, strides, offsets);

    context.IASetIndexBuffer(indexBuffer);

    context.IASetPrimitiveTopology(PrimitiveTopology::TriangleList);

    context.IASetInputLayout(inputLayout);

    context.VSSetShader(vertexShader);

    context.VSSetConstantBuffers(0, 1, &constantsBuffer);

    context.PSSetShader(pixelShader);

    context.DrawIndexed(indexCount);
}

static void CreateDeviceSwapChainBitmap(Dxgi::SwapChain1 const & swapChain,
                                        DeviceContext const & context)
{
    auto desc = swapChain.GetDescription1();
    auto width = static_cast<float>(desc.Width);
    auto height = static_cast<float>(desc.Height);

    renderTargetView = device.CreateRenderTargetView(swapChain);

    auto depthStencil = device.CreateTexture2D(TextureDescription2D{ Dxgi::Format::D24_UNORM_S8_UINT, desc.Width, desc.Height, 1, 1, BindFlag::DepthStencil });
    depthStencilView = device.CreateDepthStencilView(depthStencil, DepthStencilViewDescription{ DSVDimension::Texture2D });

    context.OMSetRenderTargets(1, &renderTargetView, depthStencilView);

    ViewPort viewPorts( 0, 0, width, height );
    context.RSSetViewports(1, &viewPorts);

    auto aspectRatio = width / height;
    auto fovAngleY = 70.0f * XM_PI / 180.0f;

    // Note that the OrientationTransform3D matrix is post-multiplied here
    // in order to correctly orient the scene to match the display orientation.
    // This post-multiplication step is required for any draw calls that are
    // made to the swap chain render target. For draw calls to other targets,
    // this transform should not be applied.

    // This sample makes use of a right-handed coordinate system using row-major matrices.
    auto perspectiveMatrix = XMMatrixPerspectiveFovRH(fovAngleY, aspectRatio, 0.01f, 100.0f);

    XMStoreFloat4x4(&constants.projection, XMMatrixTranspose(perspectiveMatrix));

    // Eye is at (0,0.7,1.5), looking at point (0,-0.1,0) with the up-vector along the y-axis.
    const XMVECTORF32 eye = { 0.0f,  0.7f, 1.5f, 0.0f };
    const XMVECTORF32 at =  { 0.0f, -0.1f, 0.0f, 0.0f };
    const XMVECTORF32 up =  { 0.0f,  1.0f, 0.0f, 0.0f };

    XMStoreFloat4x4(&constants.view, XMMatrixTranspose(XMMatrixLookAtRH(eye, at, up)));
}

static void Render(HWND window)
{
    if (!context)
    {
        device = Direct3D::CreateDevice();
        context = device.GetImmediateContext();
        auto dxgi = device.GetDxgiFactory();

        Dxgi::SwapChainDescription1 description;
        description.SwapEffect = Dxgi::SwapEffect::Discard;

        swapChain = dxgi.CreateSwapChainForHwnd(device,
                                                window,
                                                description);

        CreateDeviceSwapChainBitmap(swapChain,
                                    context);

        CreateDeviceResources();
    }

    Draw();

    auto const hr = swapChain.Present();

    if (S_OK != hr && DXGI_STATUS_OCCLUDED != hr)
    {
        ReleaseDevice();
    }
}

static void ResizeSwapChainBitmap()
{
    context.OMSetRenderTargets();
    renderTargetView.Reset();
    depthStencilView.Reset();

    if (S_OK == swapChain.ResizeBuffers())
    {
        CreateDeviceSwapChainBitmap(swapChain,
                                    context);
    }
    else
    {
        ReleaseDevice();
    }
}

int __stdcall wWinMain(HINSTANCE module, HINSTANCE, PWSTR, int)
{
    ComInitialize com;

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
            if (context && SIZE_MINIMIZED != wparam)
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
    ReleaseDeviceIndependentResources();
}
