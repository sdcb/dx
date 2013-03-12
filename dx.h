#pragma once

#include <windows.h>
#include <d2d1_1.h>
#include <d3d11_1.h>
#include <dwrite_1.h>
#include <wincodec.h>
#include <wrl.h>
#include <crtdbg.h>
#include <memory>

#pragma comment(lib, "d2d1")
#pragma comment(lib, "d3d11")
#pragma comment(lib, "dxgi")

#pragma warning(disable: 4706)
#pragma warning(disable: 4127)

#ifndef ASSERT
#define ASSERT(cond) _ASSERTE(cond)
#if WINAPI_FAMILY_PHONE_APP == WINAPI_FAMILY
#ifdef _DEBUG
#undef ASSERT
#define ASSERT(expression) { if (!(expression)) throw Platform::Exception::CreateException(E_FAIL); }
#endif
#endif
#endif

#ifndef TRACE
#ifdef _DEBUG
#include <stdio.h>
inline void TRACE(WCHAR const * const format, ...)
{
    va_list args;
    va_start(args, format);
    WCHAR output[512];
    vswprintf_s(output, format, args);
    OutputDebugString(output);
    va_end(args);
}
#else
#define TRACE __noop
#endif
#endif

namespace KennyKerr
{
    namespace Details // code in Details namespace is for internal use within the library
    {
        struct BoolStruct { int Member; };
        typedef int BoolStruct::* BoolType;

        class Object
        {
            bool operator==(Object const &);
            bool operator!=(Object const &);

        protected:

            Microsoft::WRL::ComPtr<IUnknown> m_ptr;

            Object() {}
            Object(Object const & other) : m_ptr(other.m_ptr) {}
            Object(Object && other) : m_ptr(std::move(other.m_ptr)) {}
            void Copy(Object const & other) { m_ptr = other.m_ptr; }
            void Move(Object && other) { m_ptr = std::move(other.m_ptr); }

        public:

            operator Details::BoolType() const { return nullptr != m_ptr ? &Details::BoolStruct::Member : nullptr; }
            void Reset() { m_ptr.Reset(); }
        };

        template <typename T>
        class RemoveAddRefRelease : public T
        {
            ULONG __stdcall AddRef();
            ULONG __stdcall Release();
        };

        #define KENNYKERR_DEFINE_CLASS(THIS_CLASS, BASE_CLASS, INTERFACE)                                                                      \
        THIS_CLASS() {}                                                                                                                        \
        THIS_CLASS(THIS_CLASS const & other) : BASE_CLASS(other) {}                                                                            \
        THIS_CLASS(THIS_CLASS && other)      : BASE_CLASS(std::move(other)) {}                                                                 \
        THIS_CLASS & operator=(THIS_CLASS const & other) { Copy(other);            return *this; }                                             \
        THIS_CLASS & operator=(THIS_CLASS && other)      { Move(std::move(other)); return *this; }                                             \
        Details::RemoveAddRefRelease<INTERFACE> * operator->() const { return static_cast<Details::RemoveAddRefRelease<INTERFACE> *>(Get()); } \
        auto Get() const -> INTERFACE *     {                 return static_cast<INTERFACE *>(m_ptr.Get()); }                                  \
        auto GetAddressOf() -> INTERFACE ** { ASSERT(!m_ptr); return reinterpret_cast<INTERFACE **>(m_ptr.GetAddressOf()); }

        #define KENNYKERR_DEFINE_STRUCT(STRUCT)                                                \
        auto Get() const -> STRUCT const * { return reinterpret_cast<STRUCT const *>(this); }; \
        auto Get() -> STRUCT *             { return reinterpret_cast<STRUCT *>(this);       }; \
        auto Ref() const -> STRUCT const & { return *Get();                                 }; \
        auto Ref() -> STRUCT &             { return *Get();                                 };
    }

    #ifndef __cplusplus_winrt
    struct Exception
    {
        HRESULT result;
        explicit Exception(HRESULT const value) : result(value) {}
    };
    #endif

    inline void HR(HRESULT const result)
    {
        if (S_OK != result)
        #ifndef __cplusplus_winrt
        throw Exception(result);
        #else
        throw Platform::Exception::CreateException(result);
        #endif
    }

    enum class AlphaMode // compatible with both DXGI_ALPHA_MODE and D2D1_ALPHA_MODE
    {
        Unknown      = D2D1_ALPHA_MODE_UNKNOWN,       // DXGI_ALPHA_MODE_UNSPECIFIED
        Premultipled = D2D1_ALPHA_MODE_PREMULTIPLIED, // DXGI_ALPHA_MODE_PREMULTIPLIED
        Straight     = D2D1_ALPHA_MODE_STRAIGHT,      // DXGI_ALPHA_MODE_STRAIGHT
        Ignore       = D2D1_ALPHA_MODE_IGNORE,        // DXGI_ALPHA_MODE_IGNORE
    };

    enum class ExecutionContext
    {
        InprocServer   = CLSCTX_INPROC_SERVER,
        InprocHandler  = CLSCTX_INPROC_HANDLER,
        LocalServer    = CLSCTX_LOCAL_SERVER,
        RemoteServer   = CLSCTX_REMOTE_SERVER,
        EnableCloaking = CLSCTX_ENABLE_CLOAKING,
        AppContainer   = CLSCTX_APPCONTAINER,
    };
    DEFINE_ENUM_FLAG_OPERATORS(ExecutionContext)

    enum class Apartment
    {
        SingleThreaded = COINIT_APARTMENTTHREADED,
        MultiThreaded  = COINIT_MULTITHREADED,
    };

    struct SizeU
    {
        KENNYKERR_DEFINE_STRUCT(D2D1_SIZE_U)

        SizeU(D2D1_SIZE_U const & other) :
            Width(other.width),
            Height(other.height)
        {
        }

        explicit SizeU(unsigned const width  = 0,
                       unsigned const height = 0) :
            Width(width),
            Height(height)
        {}

        unsigned Width;
        unsigned Height;
    };

    struct SizeF
    {
        KENNYKERR_DEFINE_STRUCT(D2D1_SIZE_F)

        SizeF(D2D1_SIZE_F const & other) :
            Width(other.width),
            Height(other.height)
        {
        }

        explicit SizeF(float const width = 0.0f,
                       float const height = 0.0f) :
            Width(width),
            Height(height)
        {}

        float Width;
        float Height;
    };

    struct Point2F
    {
        KENNYKERR_DEFINE_STRUCT(D2D1_POINT_2F)

        Point2F(D2D1_POINT_2F const & other) :
            X(other.x),
            Y(other.y)
        {
        }

        explicit Point2F(float const x = 0.0f,
                         float const y = 0.0f) :
            X(x),
            Y(y)
        {}

        float X;
        float Y;
    };

    struct Point2U
    {
        KENNYKERR_DEFINE_STRUCT(D2D1_POINT_2U)

        Point2U(D2D1_POINT_2U const & other) :
            X(other.x),
            Y(other.y)
        {
        }

        explicit Point2U(unsigned const x = 0,
                         unsigned const y = 0) :
            X(x),
            Y(y)
        {}

        unsigned X;
        unsigned Y;
    };

    struct RectF
    {
        KENNYKERR_DEFINE_STRUCT(D2D1_RECT_F)

        explicit RectF(float const left   = 0.0f,
                       float const top    = 0.0f,
                       float const right  = 0.0f,
                       float const bottom = 0.0f) :
            Left(left),
            Top(top),
            Right(right),
            Bottom(bottom)
        {}

        float Left;
        float Top;
        float Right;
        float Bottom;
    };

    struct RectU
    {
        KENNYKERR_DEFINE_STRUCT(D2D1_RECT_U)

        explicit RectU(unsigned const left   = 0,
                       unsigned const top    = 0,
                       unsigned const right  = 0,
                       unsigned const bottom = 0) :
            Left(left),
            Top(top),
            Right(right),
            Bottom(bottom)
        {}

        unsigned Left;
        unsigned Top;
        unsigned Right;
        unsigned Bottom;
    };

    struct Stream : Details::Object
    {
        KENNYKERR_DEFINE_CLASS(Stream, Details::Object, IStream)
    };

    struct ComInitialize
    { 
        explicit ComInitialize(Apartment apartment = Apartment::MultiThreaded)
        {
            HR(CoInitializeEx(nullptr,
                              static_cast<DWORD>(apartment)));
        }

        ~ComInitialize()
        {
            CoUninitialize();
        }
    };

    template <typename T>
    auto CoCreateInstance(REFCLSID clsid,
                          T ** result,
                          ExecutionContext context = ExecutionContext::InprocServer) -> HRESULT
    {
        return ::CoCreateInstance(clsid,
                                  nullptr, // outer
                                  static_cast<DWORD>(context),
                                  __uuidof(T),
                                  reinterpret_cast<void **>(result));
    }

    namespace Dxgi
    {
        enum class Format // DXGI_FORMAT
        {
            Unknown        = DXGI_FORMAT_UNKNOWN,
            B8G8R8A8_UNORM = DXGI_FORMAT_B8G8R8A8_UNORM,
        };

        enum class Usage // DXGI_USAGE
        {
            ShaderInput        = DXGI_USAGE_SHADER_INPUT,
            RenderTargetOutput = DXGI_USAGE_RENDER_TARGET_OUTPUT,
            BackBuffer         = DXGI_USAGE_BACK_BUFFER,
            Shared             = DXGI_USAGE_SHARED,
            ReadOnly           = DXGI_USAGE_READ_ONLY,
            DiscardOnPresent   = DXGI_USAGE_DISCARD_ON_PRESENT,
            UnorderedAccess    = DXGI_USAGE_UNORDERED_ACCESS,
        };

        enum class Scaling // DXGI_SCALING
        {
            Stretch = DXGI_SCALING_STRETCH,
            None    = DXGI_SCALING_NONE,
        };

        enum class SwapEffect // DXGI_SWAP_EFFECT
        {
            Discard        = DXGI_SWAP_EFFECT_DISCARD,
            Sequential     = DXGI_SWAP_EFFECT_SEQUENTIAL,
            FlipSequential = DXGI_SWAP_EFFECT_FLIP_SEQUENTIAL,
        };

        enum class SwapChainFlag // DXGI_SWAP_CHAIN_FLAG
        {
            NonPrerotated                = DXGI_SWAP_CHAIN_FLAG_NONPREROTATED,
            AllowModeSwitch              = DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH,
            GdiCompatible                = DXGI_SWAP_CHAIN_FLAG_GDI_COMPATIBLE,
            RestrictedContent            = DXGI_SWAP_CHAIN_FLAG_RESTRICTED_CONTENT,
            RestrictSharedResourceDriver = DXGI_SWAP_CHAIN_FLAG_RESTRICT_SHARED_RESOURCE_DRIVER,
            DisplayOnly                  = DXGI_SWAP_CHAIN_FLAG_DISPLAY_ONLY,
        };

        struct SampleProperties
        {
            KENNYKERR_DEFINE_STRUCT(DXGI_SAMPLE_DESC)

            unsigned Count;
            unsigned Quality;
        };

        struct SwapChainProperties
        {
            KENNYKERR_DEFINE_STRUCT(DXGI_SWAP_CHAIN_DESC1)

            unsigned Width;
            unsigned Height;
            Format Format;
            BOOL Stereo;
            SampleProperties Sample;
            Usage BufferUsage;
            unsigned BufferCount;
            Scaling Scaling;
            SwapEffect SwapEffect;
            AlphaMode AlphaMode;
            SwapChainFlag Flags;
        };

        struct __declspec(uuid("cafcb56c-6ac3-4889-bf47-9e23bbd260ec")) Surface : Details::Object
        {
            KENNYKERR_DEFINE_CLASS(Surface, Details::Object, IDXGISurface)
        };

        struct SwapChain : Details::Object
        {
            KENNYKERR_DEFINE_CLASS(SwapChain, Details::Object, IDXGISwapChain1)

            auto SwapChain::GetBuffer(unsigned const index = 0) const -> Surface
            {
                Surface result;

                HR((*this)->GetBuffer(index,
                                      __uuidof(result),
                                      reinterpret_cast<void **>(result.GetAddressOf())));

                return result;
            }

            auto SwapChain::ResizeBuffers(unsigned const width = 0,
                                          unsigned const height = 0) const -> HRESULT
            {
                return (*this)->ResizeBuffers(0, // all buffers
                                              width,
                                              height,
                                              DXGI_FORMAT_UNKNOWN, // preserve format
                                              0); // flags
            }

            auto SwapChain::Present(unsigned const sync = 1,
                                    unsigned const flags = 0) const -> HRESULT
            {
                return (*this)->Present(sync, flags);
            }
        };

        struct __declspec(uuid("50c83a1c-e072-4c48-87b0-3630fa36a6d0")) Factory : Details::Object
        {
            KENNYKERR_DEFINE_CLASS(Factory, Details::Object, IDXGIFactory2)

            template <typename Device>
            auto CreateSwapChainForHwnd(Device const & device, // Direct3D or Dxgi Device
                                        HWND window,
                                        SwapChainProperties const & properties) const -> SwapChain
            {
                SwapChain result;

                HR((*this)->CreateSwapChainForHwnd(device.Get(),
                                                   window,
                                                   properties.Get(),
                                                   nullptr, // windowed
                                                   nullptr, // no restrictions
                                                   result.GetAddressOf()));

                return result;
            }

            template <typename Device>
            auto CreateSwapChainForCoreWindow(Device const & device,  // Direct3D or Dxgi Device
                                              IUnknown * window,
                                              SwapChainProperties const & properties) const -> SwapChain
            {
                SwapChain result;

                HR((*this)->CreateSwapChainForCoreWindow(device.Get(),
                                                         window,
                                                         properties.Get(),
                                                         nullptr, // no restrictions
                                                         result.GetAddressOf()));

                return result;
            }

            #ifdef __cplusplus_winrt
            template <typename Device>
            auto CreateSwapChainForCoreWindow(Device const & device,  // Direct3D or Dxgi Device
                                              Windows::UI::Core::CoreWindow ^ window,
                                              SwapChainProperties const & properties) const -> SwapChain
            {
                return CreateSwapChainForCoreWindow(device,
                                                    reinterpret_cast<IUnknown *>(window),
                                                    properties);
            }
            #endif

            auto RegisterOcclusionStatusWindow(HWND window,
                                               unsigned const message = WM_USER) const -> DWORD
            {
                DWORD cookie;

                HR((*this)->RegisterOcclusionStatusWindow(window,
                                                          message,
                                                          &cookie));

                return cookie;
            }

            void UnregisterOcclusionStatus(DWORD cookie) const
            {
                (*this)->UnregisterOcclusionStatus(cookie);
            }
        };

        struct Adapter : Details::Object
        {
            KENNYKERR_DEFINE_CLASS(Adapter, Details::Object, IDXGIAdapter)

            auto GetParent() const -> Factory
            {
                Factory result;

                HR((*this)->GetParent(__uuidof(result),
                                      reinterpret_cast<void **>(result.GetAddressOf())));

                return result;
            }
        };

        struct Device : Details::Object
        {
            KENNYKERR_DEFINE_CLASS(Device, Details::Object, IDXGIDevice1)

            auto GetAdapter() const -> Adapter
            {
                Adapter result;
                HR((*this)->GetAdapter(result.GetAddressOf()));
                return result;
            }
        };

        inline auto CreateFactory() -> Factory
        {
            Factory result;

            HR(CreateDXGIFactory1(__uuidof(result),
                                  reinterpret_cast<void **>(result.GetAddressOf())));

            return result;
        }
    }

    namespace Direct3D
    {
        enum class DriverType
        {
            Unknown   = D3D_DRIVER_TYPE_UNKNOWN,
            Hardware  = D3D_DRIVER_TYPE_HARDWARE,
            Reference = D3D_DRIVER_TYPE_REFERENCE,
            Null      = D3D_DRIVER_TYPE_NULL,
            Software  = D3D_DRIVER_TYPE_SOFTWARE,
            Warp      = D3D_DRIVER_TYPE_WARP,
         };

        enum class CreateDeviceFlag
        {
            None             = 0,
            SingleThreaded   = D3D11_CREATE_DEVICE_SINGLETHREADED,
            Debug            = D3D11_CREATE_DEVICE_DEBUG,
            BgraSupport      = D3D11_CREATE_DEVICE_BGRA_SUPPORT,
            Debuggable       = D3D11_CREATE_DEVICE_DEBUGGABLE,
            PreventDebugging = D3D11_CREATE_DEVICE_PREVENT_ALTERING_LAYER_SETTINGS_FROM_REGISTRY
        };
        DEFINE_ENUM_FLAG_OPERATORS(CreateDeviceFlag);

        struct MultiThread : Details::Object
        {
            KENNYKERR_DEFINE_CLASS(MultiThread, Details::Object, ID3D10Multithread)

            void Enter() const
            {
                (*this)->Enter();
            }

            void Leave() const
            {
                (*this)->Leave();
            }

            auto SetProtected(bool protect) const -> bool
            {
                return 0 != (*this)->SetMultithreadProtected(protect);
            }

            auto GetProtected() const -> bool
            {
                return 0 != (*this)->GetMultithreadProtected();
            }
        };

        struct Device : Details::Object
        {
            KENNYKERR_DEFINE_CLASS(Device, Details::Object, ID3D11Device)

            auto AsDxgi() const -> Dxgi::Device
            {
                Dxgi::Device result;
                HR(m_ptr.CopyTo(result.GetAddressOf()));
                return result;
            }

            auto AsMultiThread() const -> MultiThread
            {
                MultiThread result;
                HR(m_ptr.CopyTo(result.GetAddressOf()));
                return result;
            }

            auto GetDxgiFactory() const -> Dxgi::Factory
            {
                return AsDxgi().GetAdapter().GetParent();
            }
        };

        inline auto CreateDevice(Device & result,
                                 DriverType const type,
                                 CreateDeviceFlag flags = CreateDeviceFlag::BgraSupport) -> HRESULT
        {
            #ifdef _DEBUG
            flags |= CreateDeviceFlag::Debug;
            #endif

            return D3D11CreateDevice(nullptr, // adapter
                                     static_cast<D3D_DRIVER_TYPE>(type),
                                     nullptr, // module
                                     static_cast<unsigned>(flags),
                                     nullptr, 0, // highest available feature level
                                     D3D11_SDK_VERSION,
                                     result.GetAddressOf(),
                                     nullptr, // actual feature level
                                     nullptr); // device context
        }

        inline auto CreateDevice(CreateDeviceFlag flags = CreateDeviceFlag::BgraSupport) -> Device
        {
            Device result;

            auto hr = CreateDevice(result,
                                   DriverType::Hardware,
                                   flags);

            if (DXGI_ERROR_UNSUPPORTED == hr)
            {
                hr = CreateDevice(result,
                                  DriverType::Warp,
                                  flags);
            }

            HR(hr);
            return result;
        }
    }

    namespace Wic
    {
        enum class BitmapDitherType
        {
            None           = WICBitmapDitherTypeNone,
            Solid          = WICBitmapDitherTypeSolid,
            Ordered4x4     = WICBitmapDitherTypeOrdered4x4,
            Ordered8x8     = WICBitmapDitherTypeOrdered8x8,
            Ordered16x16   = WICBitmapDitherTypeOrdered16x16,
            Spiral4x4      = WICBitmapDitherTypeSpiral4x4,
            Spiral8x8      = WICBitmapDitherTypeSpiral8x8,
            DualSpiral4x4  = WICBitmapDitherTypeDualSpiral4x4,
            DualSpiral8x8  = WICBitmapDitherTypeDualSpiral8x8,
            ErrorDiffusion = WICBitmapDitherTypeErrorDiffusion,
        };

        enum class BitmapPaletteType
        {
            Custom           = WICBitmapPaletteTypeCustom,
            MedianCut        = WICBitmapPaletteTypeMedianCut,
            FixedBW          = WICBitmapPaletteTypeFixedBW,
            FixedHalftone8   = WICBitmapPaletteTypeFixedHalftone8,
            FixedHalftone27  = WICBitmapPaletteTypeFixedHalftone27,
            FixedHalftone64  = WICBitmapPaletteTypeFixedHalftone64,
            FixedHalftone125 = WICBitmapPaletteTypeFixedHalftone125,
            FixedHalftone216 = WICBitmapPaletteTypeFixedHalftone216,
            FixedWebPalette  = WICBitmapPaletteTypeFixedWebPalette,
            FixedHalftone252 = WICBitmapPaletteTypeFixedHalftone252,
            FixedHalftone256 = WICBitmapPaletteTypeFixedHalftone256,
            FixedGray4       = WICBitmapPaletteTypeFixedGray4,
            FixedGray16      = WICBitmapPaletteTypeFixedGray16,
            FixedGray256     = WICBitmapPaletteTypeFixedGray256,
        };

        enum class BitmapCreateCacheOption
        {
            None     = WICBitmapNoCache,
            OnDemand = WICBitmapCacheOnDemand,
            OnLoad   = WICBitmapCacheOnLoad,
        };

        enum class BitmapEncoderCacheOption
        {
            None     = WICBitmapEncoderNoCache,
            InMemory = WICBitmapEncoderCacheInMemory,
            TempFile = WICBitmapEncoderCacheTempFile,
        };

        struct Palette : Details::Object
        {
            KENNYKERR_DEFINE_CLASS(Palette, Details::Object, IWICPalette)
        };

        struct BitmapSource : Details::Object
        {
            KENNYKERR_DEFINE_CLASS(BitmapSource, Details::Object, IWICBitmapSource)

            auto GetSize() const -> SizeU
            {
                SizeU result;
                HR((*this)->GetSize(&result.Width, &result.Height));
                return result;
            }

            void GetPixelFormat(GUID & format) const
            {
                HR((*this)->GetPixelFormat(&format));
            }

            // TODO: ...
        };

        struct __declspec(uuid("00000123-a8f2-4877-ba0a-fd2b6645fb94")) BitmapLock : Details::Object
        {
            KENNYKERR_DEFINE_CLASS(BitmapLock, Details::Object, IWICBitmapLock)
        };

        struct Bitmap : BitmapSource
        {
            KENNYKERR_DEFINE_CLASS(Bitmap, BitmapSource, IWICBitmap)
        };

        struct ColorContext : Details::Object
        {
            KENNYKERR_DEFINE_CLASS(ColorContext, Details::Object, IWICColorContext)
        };

        struct FormatConverter : BitmapSource
        {
            KENNYKERR_DEFINE_CLASS(FormatConverter, BitmapSource, IWICFormatConverter);

            void Initialize(BitmapSource const & source,
                            REFGUID format = GUID_WICPixelFormat32bppPBGRA,
                            BitmapDitherType dither = BitmapDitherType::None,
                            double alphaThresholdPercent = 0.0,
                            BitmapPaletteType paletteTranslate = BitmapPaletteType::MedianCut)
            {
                HR((*this)->Initialize(source.Get(),
                                       format,
                                       static_cast<WICBitmapDitherType>(dither),
                                       nullptr,
                                       alphaThresholdPercent,
                                       static_cast<WICBitmapPaletteType>(paletteTranslate)));
            }
        };

        struct BitmapEncoder : Details::Object
        {
            KENNYKERR_DEFINE_CLASS(BitmapEncoder, Details::Object, IWICBitmapEncoder)

            //void Initialize(Stream const & stream, WICBitmapEncoderNoCache
        };

        struct Factory : Details::Object
        {
            KENNYKERR_DEFINE_CLASS(Factory, Details::Object, IWICImagingFactory)

            auto CreateBitmap(SizeU const & size,
                              REFGUID format = GUID_WICPixelFormat32bppPBGRA,
                              BitmapCreateCacheOption cache = BitmapCreateCacheOption::OnLoad) -> Bitmap
            {
                Bitmap result;

                HR((*this)->CreateBitmap(size.Width,
                                         size.Height,
                                         format,
                                         static_cast<WICBitmapCreateCacheOption>(cache),
                                         result.GetAddressOf()));

                return result;
            }

            auto CreateEncoder(REFGUID format) -> BitmapEncoder
            {
                BitmapEncoder result;

                HR((*this)->CreateEncoder(format,
                                          nullptr,
                                          result.GetAddressOf()));

                return result;
            }
        };

        inline auto CreateFactory() -> Factory
        {
            Factory result;

            HR(CoCreateInstance(CLSID_WICImagingFactory,
                                result.GetAddressOf()));

            return result;
        }
    }

    namespace Wam
    {
    }

    namespace DirectWrite
    {
        enum class PixelGeometry
        {
            Flat = DWRITE_PIXEL_GEOMETRY_FLAT,
            Rgb  = DWRITE_PIXEL_GEOMETRY_RGB,
            Bgr  = DWRITE_PIXEL_GEOMETRY_BGR,
        };

        enum class RenderingMode
        {
            Default          = DWRITE_RENDERING_MODE_DEFAULT,
            Aliased          = DWRITE_RENDERING_MODE_ALIASED,
            GdiClassic       = DWRITE_RENDERING_MODE_GDI_CLASSIC,
            GdiNatural       = DWRITE_RENDERING_MODE_GDI_NATURAL,
            Natural          = DWRITE_RENDERING_MODE_NATURAL,
            NaturalSymmetric = DWRITE_RENDERING_MODE_NATURAL_SYMMETRIC,
            Outline          = DWRITE_RENDERING_MODE_OUTLINE,
        };

        enum class MeasuringMode
        {
            Natural    = DWRITE_MEASURING_MODE_NATURAL,
            GdiClassic = DWRITE_MEASURING_MODE_GDI_CLASSIC,
            GdiNatural = DWRITE_MEASURING_MODE_GDI_NATURAL,
        };

        struct RenderingParams : Details::Object
        {
            KENNYKERR_DEFINE_CLASS(RenderingParams, Details::Object, IDWriteRenderingParams)

            auto GetGamma() const -> float
            {
                return (*this)->GetGamma();
            }

            auto GetEnhancedContrast() const -> float
            {
                return (*this)->GetEnhancedContrast();
            }

            auto GetClearTypeLevel() const -> float
            {
                return (*this)->GetClearTypeLevel();
            }

            auto GetPixelGeometry() const -> PixelGeometry
            {
                return static_cast<PixelGeometry>((*this)->GetPixelGeometry());
            }

            auto GetRenderingMode() const -> RenderingMode
            {
                return static_cast<RenderingMode>((*this)->GetRenderingMode());
            }
        };

        struct TextFormat : Details::Object
        {
            KENNYKERR_DEFINE_CLASS(TextFormat, Details::Object, IDWriteTextFormat)
        };

        struct TextLayout : TextFormat
        {
            KENNYKERR_DEFINE_CLASS(TextLayout, TextFormat, IDWriteTextLayout)
        };
    }

    namespace Direct2D
    {
        #pragma region Enumerations

        enum class BitmapInterpolationMode
        {
            NearestNeighbor   = D2D1_BITMAP_INTERPOLATION_MODE_NEAREST_NEIGHBOR,
            Linear            = D2D1_BITMAP_INTERPOLATION_MODE_LINEAR,
        };

        enum class InterpolationMode
        {
            NearestNeighbor   = D2D1_INTERPOLATION_MODE_NEAREST_NEIGHBOR,
            Linear            = D2D1_INTERPOLATION_MODE_LINEAR,
            Cubic             = D2D1_INTERPOLATION_MODE_CUBIC,
            MultiSampleLinear = D2D1_INTERPOLATION_MODE_MULTI_SAMPLE_LINEAR,
            Anisotropic       = D2D1_INTERPOLATION_MODE_ANISOTROPIC,
            HighQualityCubic  = D2D1_INTERPOLATION_MODE_HIGH_QUALITY_CUBIC,
        };

        enum class Gamma
        {
            _2_2 = D2D1_GAMMA_2_2,
            _1_0 = D2D1_GAMMA_1_0,
        };

        enum class OpacityMaskContent
        {
            Graphics          = D2D1_OPACITY_MASK_CONTENT_GRAPHICS,
            TextNatural       = D2D1_OPACITY_MASK_CONTENT_TEXT_NATURAL,
            TextGdiCompatible = D2D1_OPACITY_MASK_CONTENT_TEXT_GDI_COMPATIBLE,
        };

        enum class DeviceContextOptions
        {
            None                             = D2D1_DEVICE_CONTEXT_OPTIONS_NONE,
            EnableMultiThreadedOptimizations = D2D1_DEVICE_CONTEXT_OPTIONS_ENABLE_MULTITHREADED_OPTIMIZATIONS,
        };

        enum class BitmapOptions
        {
            None          = D2D1_BITMAP_OPTIONS_NONE,
            Target        = D2D1_BITMAP_OPTIONS_TARGET,
            CannotDraw    = D2D1_BITMAP_OPTIONS_CANNOT_DRAW,
            CpuRead       = D2D1_BITMAP_OPTIONS_CPU_READ,
            GdiCompatible = D2D1_BITMAP_OPTIONS_GDI_COMPATIBLE,
        };
        DEFINE_ENUM_FLAG_OPERATORS(BitmapOptions)

        enum class CapStyle
        {
            Flat     = D2D1_CAP_STYLE_FLAT,
            Square   = D2D1_CAP_STYLE_SQUARE,
            Round    = D2D1_CAP_STYLE_ROUND,
            Triangle = D2D1_CAP_STYLE_TRIANGLE,
        };

        enum class LineJoin
        {
            Miter        = D2D1_LINE_JOIN_MITER,
            Bevel        = D2D1_LINE_JOIN_BEVEL,
            Round        = D2D1_LINE_JOIN_ROUND,
            MiterOrBevel = D2D1_LINE_JOIN_MITER_OR_BEVEL,
        };

        enum class DashStyle
        {
            Solid      = D2D1_DASH_STYLE_SOLID,
            Dash       = D2D1_DASH_STYLE_DASH,
            Dot        = D2D1_DASH_STYLE_DOT,
            DashDot    = D2D1_DASH_STYLE_DASH_DOT,
            DashDotDot = D2D1_DASH_STYLE_DASH_DOT_DOT,
            Custom     = D2D1_DASH_STYLE_CUSTOM,
        };

        enum class StrokeTransformType
        {
            Normal   = D2D1_STROKE_TRANSFORM_TYPE_NORMAL,
            Fixed    = D2D1_STROKE_TRANSFORM_TYPE_FIXED,
            Hairline = D2D1_STROKE_TRANSFORM_TYPE_HAIRLINE,
        };

        enum class UnitMode
        {
            Dips   = D2D1_UNIT_MODE_DIPS,
            Pixels = D2D1_UNIT_MODE_PIXELS,
        };

        enum class CompositeMode
        {
            SourceOver        = D2D1_COMPOSITE_MODE_SOURCE_OVER,
            DestinationOver   = D2D1_COMPOSITE_MODE_DESTINATION_OVER,
            SourceIn          = D2D1_COMPOSITE_MODE_SOURCE_IN,
            DestinationIn     = D2D1_COMPOSITE_MODE_DESTINATION_IN,
            SourceOut         = D2D1_COMPOSITE_MODE_SOURCE_OUT,
            DestinationOut    = D2D1_COMPOSITE_MODE_DESTINATION_OUT,
            SourceAtop        = D2D1_COMPOSITE_MODE_SOURCE_ATOP,
            DestinationAtop   = D2D1_COMPOSITE_MODE_DESTINATION_ATOP,
            Xor               = D2D1_COMPOSITE_MODE_XOR,
            Plus              = D2D1_COMPOSITE_MODE_PLUS,
            SourceCopy        = D2D1_COMPOSITE_MODE_SOURCE_COPY,
            BoundedSourceCopy = D2D1_COMPOSITE_MODE_BOUNDED_SOURCE_COPY,
            MaskInvert        = D2D1_COMPOSITE_MODE_MASK_INVERT,
        };

        enum class ExtendMode
        {
            Clamp  = D2D1_EXTEND_MODE_CLAMP,
            Wrap   = D2D1_EXTEND_MODE_WRAP,
            Mirror = D2D1_EXTEND_MODE_MIRROR,
        };

        enum class AntialiasMode
        {
            PerPrimitive = D2D1_ANTIALIAS_MODE_PER_PRIMITIVE,
            Aliased      = D2D1_ANTIALIAS_MODE_ALIASED,
        };

        enum class TextAntialiasMode
        {
            Default   = D2D1_TEXT_ANTIALIAS_MODE_DEFAULT,
            ClearType = D2D1_TEXT_ANTIALIAS_MODE_CLEARTYPE,
            Grayscale = D2D1_TEXT_ANTIALIAS_MODE_GRAYSCALE,
            Aliased   = D2D1_TEXT_ANTIALIAS_MODE_ALIASED,
        };

        enum class DrawTextOptions
        {
            NoSnap = D2D1_DRAW_TEXT_OPTIONS_NO_SNAP,
            Clip   = D2D1_DRAW_TEXT_OPTIONS_CLIP,
            None   = D2D1_DRAW_TEXT_OPTIONS_NONE,
        };
        DEFINE_ENUM_FLAG_OPERATORS(DrawTextOptions);

        enum class ArcSize
        {
            Small = D2D1_ARC_SIZE_SMALL,
            Large = D2D1_ARC_SIZE_LARGE,
        };

        enum class CombineMode
        {
            Union     = D2D1_COMBINE_MODE_UNION,
            Intersect = D2D1_COMBINE_MODE_INTERSECT,
            Xor       = D2D1_COMBINE_MODE_XOR,
            Exclude   = D2D1_COMBINE_MODE_EXCLUDE,
        };

        enum class GeometryRelation
        {
            Unknown     = D2D1_GEOMETRY_RELATION_UNKNOWN,
            Disjoint    = D2D1_GEOMETRY_RELATION_DISJOINT,
            IsContained = D2D1_GEOMETRY_RELATION_IS_CONTAINED,
            Contains    = D2D1_GEOMETRY_RELATION_CONTAINS,
            Overlap     = D2D1_GEOMETRY_RELATION_OVERLAP,
        };

        enum class GeometrySimplificationOption
        {
            CubicsAndLines = D2D1_GEOMETRY_SIMPLIFICATION_OPTION_CUBICS_AND_LINES,
            Lines          = D2D1_GEOMETRY_SIMPLIFICATION_OPTION_LINES,
        };

        enum class FigureBegin
        {
            Filled = D2D1_FIGURE_BEGIN_FILLED,
            Hollow = D2D1_FIGURE_BEGIN_HOLLOW,
        };

        enum class FigureEnd
        {
            Open   = D2D1_FIGURE_END_OPEN,
            Closed = D2D1_FIGURE_END_CLOSED,
        };

        enum class PathSegment
        {
            None               = D2D1_PATH_SEGMENT_NONE,
            ForceUnstroked     = D2D1_PATH_SEGMENT_FORCE_UNSTROKED,
            ForceRoundLineJoin = D2D1_PATH_SEGMENT_FORCE_ROUND_LINE_JOIN,
        };
        DEFINE_ENUM_FLAG_OPERATORS(PathSegment)

        enum class SweepDirection
        {
            CounterClockwise = D2D1_SWEEP_DIRECTION_COUNTER_CLOCKWISE,
            Clockwise        = D2D1_SWEEP_DIRECTION_CLOCKWISE,
        };

        enum class FillMode
        {
            Alternate = D2D1_FILL_MODE_ALTERNATE,
            Winding = D2D1_FILL_MODE_WINDING,
        };

        enum class LayerOptions
        {
            None                     = D2D1_LAYER_OPTIONS1_NONE,
            InitializeFromBackground = D2D1_LAYER_OPTIONS1_INITIALIZE_FROM_BACKGROUND,
            IgnoreAlpha              = D2D1_LAYER_OPTIONS1_IGNORE_ALPHA,
        };
        DEFINE_ENUM_FLAG_OPERATORS(LayerOptions)

        enum class WindowState
        {
            None     = D2D1_WINDOW_STATE_NONE,
            Occluded = D2D1_WINDOW_STATE_OCCLUDED,
        };
        DEFINE_ENUM_FLAG_OPERATORS(WindowState)

        enum class RenderTargetType
        {
            Default  = D2D1_RENDER_TARGET_TYPE_DEFAULT,
            Software = D2D1_RENDER_TARGET_TYPE_SOFTWARE,
            Hardware = D2D1_RENDER_TARGET_TYPE_HARDWARE,
        };

        enum class FeatureLevel
        {
            Default = D2D1_FEATURE_LEVEL_DEFAULT,
            _9      = D2D1_FEATURE_LEVEL_9,
            _10     = D2D1_FEATURE_LEVEL_10,
        };

        enum class RenderTargetUsage
        {
            None                = D2D1_RENDER_TARGET_USAGE_NONE,
            ForceBitmapRemoting = D2D1_RENDER_TARGET_USAGE_FORCE_BITMAP_REMOTING,
            GdiCompatible       = D2D1_RENDER_TARGET_USAGE_GDI_COMPATIBLE,
        };
        DEFINE_ENUM_FLAG_OPERATORS(RenderTargetUsage);

        enum class PresentOptions
        {
            None = D2D1_PRESENT_OPTIONS_NONE,
            RetainContents = D2D1_PRESENT_OPTIONS_RETAIN_CONTENTS,
            Immediately = D2D1_PRESENT_OPTIONS_IMMEDIATELY,
        };
        DEFINE_ENUM_FLAG_OPERATORS(PresentOptions);

        enum class CompatibleRenderTargetOptions
        {
            None          = D2D1_COMPATIBLE_RENDER_TARGET_OPTIONS_NONE,
            GdiCompatible = D2D1_COMPATIBLE_RENDER_TARGET_OPTIONS_GDI_COMPATIBLE,
        };

        enum class DcInitializeMode
        {
            Copy  = D2D1_DC_INITIALIZE_MODE_COPY,
            Clear = D2D1_DC_INITIALIZE_MODE_CLEAR,
        };

        enum class PropertyType
        {
            Unknown      = D2D1_PROPERTY_TYPE_UNKNOWN,
            String       = D2D1_PROPERTY_TYPE_STRING,
            Bool         = D2D1_PROPERTY_TYPE_BOOL,
            Uint32       = D2D1_PROPERTY_TYPE_UINT32,
            Int32        = D2D1_PROPERTY_TYPE_INT32,
            Float        = D2D1_PROPERTY_TYPE_FLOAT,
            Vector2      = D2D1_PROPERTY_TYPE_VECTOR2,
            Vector3      = D2D1_PROPERTY_TYPE_VECTOR3,
            Vector4      = D2D1_PROPERTY_TYPE_VECTOR4,
            Blob         = D2D1_PROPERTY_TYPE_BLOB,
            IUnknown     = D2D1_PROPERTY_TYPE_IUNKNOWN,
            Enum         = D2D1_PROPERTY_TYPE_ENUM,
            Array        = D2D1_PROPERTY_TYPE_ARRAY,
            Clsid        = D2D1_PROPERTY_TYPE_CLSID,
            Matrix3X2    = D2D1_PROPERTY_TYPE_MATRIX_3X2,
            Matrix4X3    = D2D1_PROPERTY_TYPE_MATRIX_4X3,
            Matrix4X4    = D2D1_PROPERTY_TYPE_MATRIX_4X4,
            Matrix5X4    = D2D1_PROPERTY_TYPE_MATRIX_5X4,
            ColorContent = D2D1_PROPERTY_TYPE_COLOR_CONTEXT,
        };

        enum class Property
        {
            Clsid       = D2D1_PROPERTY_CLSID,
            DisplayName = D2D1_PROPERTY_DISPLAYNAME,
            Author      = D2D1_PROPERTY_AUTHOR,
            Category    = D2D1_PROPERTY_CATEGORY,
            Description = D2D1_PROPERTY_DESCRIPTION,
            Inputs      = D2D1_PROPERTY_INPUTS,
            Cached      = D2D1_PROPERTY_CACHED,
            Precision   = D2D1_PROPERTY_PRECISION,
            MinInputs   = D2D1_PROPERTY_MIN_INPUTS,
            MaxInputs   = D2D1_PROPERTY_MAX_INPUTS,
        };

        enum class SubProperty
        {
            DisplayName = D2D1_SUBPROPERTY_DISPLAYNAME,
            IsReadOnly  = D2D1_SUBPROPERTY_ISREADONLY,
            Min         = D2D1_SUBPROPERTY_MIN,
            Max         = D2D1_SUBPROPERTY_MAX,
            Default     = D2D1_SUBPROPERTY_DEFAULT,
            Fields      = D2D1_SUBPROPERTY_FIELDS,
            Index       = D2D1_SUBPROPERTY_INDEX,
        };

        enum class BufferPrecision
        {
            Unknown          = D2D1_BUFFER_PRECISION_UNKNOWN,
            _8BPC_UNORM      = D2D1_BUFFER_PRECISION_8BPC_UNORM,
            _8BPC_UNORM_SRGB = D2D1_BUFFER_PRECISION_8BPC_UNORM_SRGB,
            _16BPC_UNORM     = D2D1_BUFFER_PRECISION_16BPC_UNORM,
            _16BPC_FLOAT     = D2D1_BUFFER_PRECISION_16BPC_FLOAT,
            _32BPC_FLOAT     = D2D1_BUFFER_PRECISION_32BPC_FLOAT,
        };

        enum class MapOptions
        {
            None    = D2D1_MAP_OPTIONS_NONE,
            Read    = D2D1_MAP_OPTIONS_READ,
            Write   = D2D1_MAP_OPTIONS_WRITE,
            Discard = D2D1_MAP_OPTIONS_DISCARD,
        };
        DEFINE_ENUM_FLAG_OPERATORS(MapOptions)

        enum class ColorSpace
        {
            Custom = D2D1_COLOR_SPACE_CUSTOM,
            sRGB   = D2D1_COLOR_SPACE_SRGB,
            scRGB  = D2D1_COLOR_SPACE_SCRGB,
        };

        enum class PrimitiveBlend
        {
            SourceOver = D2D1_PRIMITIVE_BLEND_SOURCE_OVER,
            Copy       = D2D1_PRIMITIVE_BLEND_COPY,
        };

        enum class FactoryType
        {
            SingleThreaded = D2D1_FACTORY_TYPE_SINGLE_THREADED,
            MultiThreaded  = D2D1_FACTORY_TYPE_MULTI_THREADED,
        };

        enum class ThreadingMode
        {
            SingleThreaded = D2D1_THREADING_MODE_SINGLE_THREADED,
            MultiThreaded  = D2D1_THREADING_MODE_MULTI_THREADED,
        };

        enum class ColorInterpolationMode
        {
            Straight      = D2D1_COLOR_INTERPOLATION_MODE_STRAIGHT,
            Premultiplied = D2D1_COLOR_INTERPOLATION_MODE_PREMULTIPLIED,
        };

        enum class PrintFontSubsetMode
        {
            Default  = D2D1_PRINT_FONT_SUBSET_MODE_DEFAULT,
            EachPage = D2D1_PRINT_FONT_SUBSET_MODE_EACHPAGE,
            None     = D2D1_PRINT_FONT_SUBSET_MODE_NONE,
        };

        enum class DebugLevel
        {
            None        = D2D1_DEBUG_LEVEL_NONE,
            Error       = D2D1_DEBUG_LEVEL_ERROR,
            Warning     = D2D1_DEBUG_LEVEL_WARNING,
            Information = D2D1_DEBUG_LEVEL_INFORMATION,
        };

        #pragma endregion

        #pragma region Structures

        struct DrawingStateDescription
        {
            KENNYKERR_DEFINE_STRUCT(D2D1_DRAWING_STATE_DESCRIPTION)

            DrawingStateDescription(AntialiasMode antialiasMode = AntialiasMode::PerPrimitive,
                                    TextAntialiasMode textAntialiasMode = TextAntialiasMode::Default,
                                    UINT64 tag1 = 0,
                                    UINT64 tag2 = 0,
                                    D2D1_MATRIX_3X2_F const & transform = D2D1::IdentityMatrix()) :
                AntialiasMode(antialiasMode),
                TextAntialiasMode(textAntialiasMode),
                Tag1(tag1),
                Tag2(tag2),
                Transform(transform)
            {
            }

            AntialiasMode AntialiasMode;
            TextAntialiasMode TextAntialiasMode;
            UINT64 Tag1;
            UINT64 Tag2;
            D2D1_MATRIX_3X2_F Transform;
        };

        struct DrawingStateDescription1
        {
            KENNYKERR_DEFINE_STRUCT(D2D1_DRAWING_STATE_DESCRIPTION1)

            DrawingStateDescription1(AntialiasMode antialiasMode = AntialiasMode::PerPrimitive,
                                     TextAntialiasMode textAntialiasMode = TextAntialiasMode::Default,
                                     UINT64 tag1 = 0,
                                     UINT64 tag2 = 0,
                                     D2D1_MATRIX_3X2_F const & transform = D2D1::IdentityMatrix(),
                                     PrimitiveBlend const & primitiveBlend = PrimitiveBlend::SourceOver,
                                     UnitMode unitMode = UnitMode::Dips) :
                AntialiasMode(antialiasMode),
                TextAntialiasMode(textAntialiasMode),
                Tag1(tag1),
                Tag2(tag2),
                Transform(transform),
                PrimitiveBlend(primitiveBlend),
                UnitMode(unitMode)
            {
            }

            AntialiasMode AntialiasMode;
            TextAntialiasMode TextAntialiasMode;
            UINT64 Tag1;
            UINT64 Tag2;
            D2D1_MATRIX_3X2_F Transform;
            PrimitiveBlend PrimitiveBlend;
            UnitMode UnitMode;
        };

        struct Color
        {
            KENNYKERR_DEFINE_STRUCT(D2D1_COLOR_F)

            Color(D2D1_COLOR_F const & other) :
                Red(other.r),
                Green(other.g),
                Blue(other.b),
                Alpha(other.a)
            {
            }

            explicit Color(float red   = 0.0f,
                           float green = 0.0f,
                           float blue  = 0.0f,
                           float alpha = 1.0f) :
                Red(red),
                Green(green),
                Blue(blue),
                Alpha(alpha)
            {
            }

            float Red;
            float Green;
            float Blue;
            float Alpha;
        };

        struct ArcSegment
        {
            KENNYKERR_DEFINE_STRUCT(D2D1_ARC_SEGMENT)

            explicit ArcSegment(Point2F const & point = Point2F(),
                                SizeF const & size = SizeF(),
                                float rotationAngle = 0.0f,
                                SweepDirection sweepDirection = SweepDirection::Clockwise,
                                ArcSize arcSize = ArcSize::Small) :
                Point(point),
                Size(size),
                RotationAngle(rotationAngle),
                SweepDirection(sweepDirection),
                ArcSize(arcSize)
            {
            }

            Point2F Point;
            SizeF Size;
            float RotationAngle;
            SweepDirection SweepDirection;
            ArcSize ArcSize;
        };

        struct BezierSegment
        {
            KENNYKERR_DEFINE_STRUCT(D2D1_BEZIER_SEGMENT)

            explicit BezierSegment(Point2F const & point1 = Point2F(),
                                   Point2F const & point2 = Point2F(),
                                   Point2F const & point3 = Point2F()) :
                Point1(point1),
                Point2(point2),
                Point3(point3)
            {
            }

            Point2F Point1;
            Point2F Point2;
            Point2F Point3;
        };

        struct QuadraticBezierSegment
        {
            KENNYKERR_DEFINE_STRUCT(D2D1_QUADRATIC_BEZIER_SEGMENT)

            explicit QuadraticBezierSegment(Point2F const & point1 = Point2F(),
                                            Point2F const & point2 = Point2F()) :
                Point1(point1),
                Point2(point2)
            {
            }

            Point2F Point1;
            Point2F Point2;
        };

        struct Triangle
        {
            KENNYKERR_DEFINE_STRUCT(D2D1_TRIANGLE)

            explicit Triangle(Point2F const & point1 = Point2F(),
                              Point2F const & point2 = Point2F(),
                              Point2F const & point3 = Point2F()) :
                Point1(point1),
                Point2(point2),
                Point3(point3)
            {
            }

            Point2F Point1;
            Point2F Point2;
            Point2F Point3;
        };

        struct RoundedRect
        {
            KENNYKERR_DEFINE_STRUCT(D2D1_ROUNDED_RECT)

            explicit RoundedRect(RectF const & rect = RectF(),
                                 float radiusX = 0.0f,
                                 float radiusY = 0.0f) :
                Rect(rect),
                RadiusX(radiusX),
                RadiusY(radiusY)
            {}

            RectF Rect;
            float RadiusX;
            float RadiusY;
        };

        struct Ellipse
        {
            KENNYKERR_DEFINE_STRUCT(D2D1_ELLIPSE)

            explicit Ellipse(Point2F const & center = Point2F(),
                             float radiusX = 0.0f,
                             float radiusY = 0.0f) :
                Center(center),
                RadiusX(radiusX),
                RadiusY(radiusY)
            {}

            Point2F Center;
            float RadiusX;
            float RadiusY;
        };

        struct GradientStop
        {
            KENNYKERR_DEFINE_STRUCT(D2D1_GRADIENT_STOP)

            explicit GradientStop(float position = 0.0f,
                                  Color const & color = Direct2D::Color()) :
                Position(position),
                Color(color)
            {
            }

            float Position;
            Color Color;
        };

        struct PixelFormat
        {
            KENNYKERR_DEFINE_STRUCT(D2D1_PIXEL_FORMAT)

            PixelFormat(D2D1_PIXEL_FORMAT const & other) :
                Format(static_cast<Dxgi::Format>(other.format)),
                AlphaMode(static_cast<KennyKerr::AlphaMode>(other.alphaMode))
            {
            }

            explicit PixelFormat(Dxgi::Format format = Dxgi::Format::B8G8R8A8_UNORM,
                                 AlphaMode mode = AlphaMode::Premultipled) :
                Format(format),
                AlphaMode(mode)
            {
            }

            Dxgi::Format Format;
            AlphaMode AlphaMode;
        };

        struct PrintControlProperties
        {
            KENNYKERR_DEFINE_STRUCT(D2D1_PRINT_CONTROL_PROPERTIES)

            explicit PrintControlProperties(PrintFontSubsetMode fontSubset = PrintFontSubsetMode::Default,
                                            float rasterDpi = 150.0f,
                                            ColorSpace colorSpace = ColorSpace::sRGB) :
                FontSubset(fontSubset),
                RasterDpi(rasterDpi),
                ColorSpace(colorSpace)
            {
            }

            PrintFontSubsetMode FontSubset;
            float RasterDpi;
            ColorSpace ColorSpace;
        };

        struct CreationProperties
        {
            KENNYKERR_DEFINE_STRUCT(D2D1_CREATION_PROPERTIES)

            CreationProperties(ThreadingMode threadingMode,
                               DebugLevel debugLevel,
                               DeviceContextOptions options) :
                ThreadingMode(threadingMode),
                DebugLevel(debugLevel),
                Options(options)
            {
            }

            ThreadingMode ThreadingMode;
            DebugLevel DebugLevel;
            DeviceContextOptions Options;
        };

        struct BrushProperties
        {
            KENNYKERR_DEFINE_STRUCT(D2D1_BRUSH_PROPERTIES)

            explicit BrushProperties(float opacity = 1.0,
                                     D2D1_MATRIX_3X2_F const & transform = D2D1::IdentityMatrix()) :
                Opacity(opacity),
                Transform(transform)
            {}

            float Opacity;
            D2D1_MATRIX_3X2_F Transform;
        };

        struct ImageBrushProperties
        {
            KENNYKERR_DEFINE_STRUCT(D2D1_IMAGE_BRUSH_PROPERTIES)

            explicit ImageBrushProperties(RectF const & sourceRectangle,
                                          ExtendMode extendModeX = ExtendMode::Clamp,
                                          ExtendMode extendModeY = ExtendMode::Clamp,
                                          InterpolationMode interpolationMode = InterpolationMode::Linear) :
                SourceRectangle(sourceRectangle),
                ExtendModeX(extendModeX),
                ExtendModeY(extendModeY),
                InterpolationMode(interpolationMode)
            {}

            RectF SourceRectangle;
            ExtendMode ExtendModeX;
            ExtendMode ExtendModeY;
            InterpolationMode InterpolationMode;
        };

        struct BitmapProperties
        {
            KENNYKERR_DEFINE_STRUCT(D2D1_BITMAP_PROPERTIES)

            explicit BitmapProperties(PixelFormat format = Direct2D::PixelFormat(),
                                      float dpiX = 0.0f,
                                      float dpiY = 0.0f) :
                PixelFormat(format),
                DpiX(dpiX),
                DpiY(dpiY)
            {
            }

            PixelFormat PixelFormat;
            float DpiX;
            float DpiY;
        };

        struct BitmapProperties1
        {
            KENNYKERR_DEFINE_STRUCT(D2D1_BITMAP_PROPERTIES1)

            explicit BitmapProperties1(BitmapOptions options = BitmapOptions::None,
                                       PixelFormat format = Direct2D::PixelFormat(),
                                       float dpiX = 0.0f,
                                       float dpiY = 0.0f,
                                       ID2D1ColorContext * context = nullptr) :
                PixelFormat(format),
                DpiX(dpiX),
                DpiY(dpiY),
                BitmapOptions(options),
                ColorContext(context)
            {
            }

            PixelFormat PixelFormat;
            float DpiX;
            float DpiY;
            BitmapOptions BitmapOptions;
            ID2D1ColorContext * ColorContext;
        };

        struct BitmapBrushProperties
        {
            KENNYKERR_DEFINE_STRUCT(D2D1_BITMAP_BRUSH_PROPERTIES)

            explicit BitmapBrushProperties(ExtendMode extendModeX = ExtendMode::Clamp,
                                           ExtendMode extendModeY = ExtendMode::Clamp,
                                           BitmapInterpolationMode interpolationMode = BitmapInterpolationMode::Linear) :
                ExtendModeX(extendModeX),
                ExtendModeY(extendModeY),
                InterpolationMode(interpolationMode)
            {
            }

            ExtendMode ExtendModeX;
            ExtendMode ExtendModeY;
            BitmapInterpolationMode InterpolationMode;
        };

        struct BitmapBrushProperties1
        {
            KENNYKERR_DEFINE_STRUCT(D2D1_BITMAP_BRUSH_PROPERTIES1)

            explicit BitmapBrushProperties1(ExtendMode extendModeX = ExtendMode::Clamp,
                                            ExtendMode extendModeY = ExtendMode::Clamp,
                                            InterpolationMode interpolationMode = InterpolationMode::Linear) :
                ExtendModeX(extendModeX),
                ExtendModeY(extendModeY),
                InterpolationMode(interpolationMode)
            {
            }

            ExtendMode ExtendModeX;
            ExtendMode ExtendModeY;
            InterpolationMode InterpolationMode;
        };

        struct LinearGradientBrushProperties
        {
            KENNYKERR_DEFINE_STRUCT(D2D1_LINEAR_GRADIENT_BRUSH_PROPERTIES)

            explicit LinearGradientBrushProperties(Point2F startPoint = Point2F(),
                                                   Point2F endPoint = Point2F()) :
                StartPoint(startPoint),
                EndPoint(endPoint)
            {
            }

            Point2F StartPoint;
            Point2F EndPoint;
        };

        struct RadialGradientBrushProperties
        {
            KENNYKERR_DEFINE_STRUCT(D2D1_RADIAL_GRADIENT_BRUSH_PROPERTIES)

            explicit RadialGradientBrushProperties(Point2F const & center = Point2F(),
                                                   Point2F const & offset = Point2F(),
                                                   float radiusX = 0.0f,
                                                   float radiusY = 0.0f) :
                Center(center),
                Offset(offset),
                RadiusX(radiusX),
                RadiusY(radiusY)
            {
            }

            Point2F Center;
            Point2F Offset;
            float RadiusX;
            float RadiusY;
        };

        struct StrokeStyleProperties // compatible with both D2D1_STROKE_STYLE_PROPERTIES and D2D1_STROKE_STYLE_PROPERTIES1
        {
            KENNYKERR_DEFINE_STRUCT(D2D1_STROKE_STYLE_PROPERTIES1)

            explicit StrokeStyleProperties(CapStyle startCap                 = CapStyle::Flat,
                                           CapStyle endCap                   = CapStyle::Flat,
                                           CapStyle dashCap                  = CapStyle::Flat,
                                           LineJoin lineJoin                 = LineJoin::Miter,
                                           float miterLimit                  = 10.0f,
                                           DashStyle dashStyle               = DashStyle::Solid,
                                           float dashOffset                  = 0.0f,
                                           StrokeTransformType transformType = StrokeTransformType::Normal) :
                StartCap(startCap),
                EndCap(endCap),
                DashCap(dashCap),
                LineJoin(lineJoin),
                MiterLimit(miterLimit),
                DashStyle(dashStyle),
                DashOffset(dashOffset),
                TransformType(transformType)
            {
            }

            CapStyle StartCap;
            CapStyle EndCap;
            CapStyle DashCap;
            LineJoin LineJoin;
            float MiterLimit;
            DashStyle DashStyle;
            float DashOffset;
            StrokeTransformType TransformType;
        };

        struct LayerProperties
        {
            KENNYKERR_DEFINE_STRUCT(D2D1_LAYER_PARAMETERS)

            explicit LayerProperties(RectF const & contentBounds = RectF(),
                                     ID2D1Geometry * geometricMask = nullptr,
                                     AntialiasMode maskAntialiasMode = AntialiasMode::PerPrimitive,
                                     D2D1_MATRIX_3X2_F const & maskTransform = D2D1::IdentityMatrix(),
                                     float opacity = 0.0f,
                                     ID2D1Brush * opacityBrush = nullptr,
                                     LayerOptions layerOptions = LayerOptions::None) :
                ContentBounds(contentBounds),
                GeometricMask(geometricMask),
                MaskAntialiasMode(maskAntialiasMode),
                MaskTransform(maskTransform),
                Opacity(opacity),
                OpacityBrush(opacityBrush),
                LayerOptions(layerOptions)
            {
            }

            RectF ContentBounds;
            ID2D1Geometry * GeometricMask;
            AntialiasMode MaskAntialiasMode;
            D2D1_MATRIX_3X2_F MaskTransform;
            float Opacity;
            ID2D1Brush * OpacityBrush;
            LayerOptions LayerOptions;
        };

        struct RenderTargetProperties
        {
            KENNYKERR_DEFINE_STRUCT(D2D1_RENDER_TARGET_PROPERTIES)

            explicit RenderTargetProperties(RenderTargetType type = RenderTargetType::Default,
                                            PixelFormat pixelFormat = Direct2D::PixelFormat(),
                                            float dpiX = 0.0f,
                                            float dpiY = 0.0f,
                                            RenderTargetUsage usage = RenderTargetUsage::None,
                                            FeatureLevel minLevel = FeatureLevel::Default) :
                Type(type),
                PixelFormat(pixelFormat),
                DpiX(dpiX),
                DpiY(dpiY),
                Usage(usage),
                MinLevel(minLevel)
            {
            }

            RenderTargetType Type;
            PixelFormat PixelFormat;
            float DpiX;
            float DpiY;
            RenderTargetUsage Usage;
            FeatureLevel MinLevel;
        };

        struct HwndRenderTargetProperties
        {
            KENNYKERR_DEFINE_STRUCT(D2D1_HWND_RENDER_TARGET_PROPERTIES)

            explicit HwndRenderTargetProperties(HWND hwnd = nullptr,
                                                SizeU const & pixelSize = SizeU(),
                                                PresentOptions presentOptions = PresentOptions::None) :
                Hwnd(hwnd),
                PixelSize(pixelSize),
                PresentOptions(presentOptions)
            {
            }

            HWND Hwnd;
            SizeU PixelSize;
            PresentOptions PresentOptions;
        };

        struct MappedRect
        {
            KENNYKERR_DEFINE_STRUCT(D2D1_MAPPED_RECT)

            unsigned Pitch;
            BYTE * Bits;
        };

        struct RenderingControls
        {
            KENNYKERR_DEFINE_STRUCT(D2D1_RENDERING_CONTROLS)

            BufferPrecision BufferPrecision;
            SizeU TileSize;
        };

        struct EffectInputDescription
        {
            KENNYKERR_DEFINE_STRUCT(D2D1_EFFECT_INPUT_DESCRIPTION)

            ID2D1Effect * Effect;
            unsigned InputIndex;
            RectF InputRectangle;
        };

        struct PointDescription
        {
            KENNYKERR_DEFINE_STRUCT(D2D1_POINT_DESCRIPTION)

            Point2F Point;
            Point2F UnitTangentVector;
            unsigned EndSegment;
            unsigned EndFigure;
            float LengthToEndSegment;
        };

        #pragma endregion

        #pragma region Classes

        struct Factory;
        struct RenderTarget;
        struct BitmapRenderTarget;
        struct Device;

        struct SimplifiedGeometrySink : Details::Object
        {
            KENNYKERR_DEFINE_CLASS(SimplifiedGeometrySink, Details::Object, ID2D1SimplifiedGeometrySink)

            void SetFillMode(FillMode mode) const
            {
                (*this)->SetFillMode(static_cast<D2D1_FILL_MODE>(mode));
            }

            void SetSegmentFlags(PathSegment flags) const
            {
                (*this)->SetSegmentFlags(static_cast<D2D1_PATH_SEGMENT>(flags));
            }

            void BeginFigure(Point2F const & startPoint,
                             FigureBegin figureBegin) const
            {
                (*this)->BeginFigure(startPoint.Ref(),
                                     static_cast<D2D1_FIGURE_BEGIN>(figureBegin));
            }

            void AddLines(Point2F const * points,
                          unsigned count) const
            {
                ASSERT(points);
                ASSERT(count);

                (*this)->AddLines(points->Get(),
                                  count);
            }

            template <size_t Count>
            void AddLines(Point2F const (&points)[Count]) const
            {
                AddLines(points,
                         Count);
            }

            void AddBeziers(BezierSegment const * beziers,
                            unsigned count) const
            {
                ASSERT(beziers);
                ASSERT(count);

                (*this)->AddBeziers(beziers->Get(),
                                    count);
            }

            template <size_t Count>
            void AddBeziers(BezierSegment const (&beziers)[Count]) const
            {
                AddBeziers(beziers,
                           Count);
            }

            void EndFigure(FigureEnd figureEnd) const
            {
                (*this)->EndFigure(static_cast<D2D1_FIGURE_END>(figureEnd));
            }
        };

        struct TessellationSink : Details::Object
        {
            KENNYKERR_DEFINE_CLASS(TessellationSink, Details::Object, ID2D1TessellationSink)

            void AddTriangles(Triangle const * triangles,
                              unsigned count) const
            {
                ASSERT(triangles);
                ASSERT(count);

                (*this)->AddTriangles(triangles->Get(),
                                      count);
            }

            template <size_t Count>
            void AddTriangles(Triangle const (&triangles)[Count]) const
            {
                AddTriangles(triangles,
                             Count);
            }

            void Close()
            {
                HR((*this)->Close());
            }
        };

        struct Resource : Details::Object
        {
            KENNYKERR_DEFINE_CLASS(Resource, Details::Object, ID2D1Resource)

            auto GetFactory() const -> Factory;
        };

        struct Image : Resource
        {
            KENNYKERR_DEFINE_CLASS(Image, Resource, ID2D1Image)
        };

        struct __declspec(uuid("a2296057-ea42-4099-983b-539fb6505426")) Bitmap : Image
        {
            KENNYKERR_DEFINE_CLASS(Bitmap, Image, ID2D1Bitmap)

            auto GetSize() const -> SizeF
            {
                return (*this)->GetSize();
            }

            auto GetPixelSize() const -> SizeU
            {
                return (*this)->GetPixelSize();
            }

            auto GetPixelFormat() const -> PixelFormat
            {
                return (*this)->GetPixelFormat();
            }

            void GetDpi(float & x, float & y) const
            {
                (*this)->GetDpi(&x, &y);
            }

            void CopyFromBitmap(Bitmap const & other) const
            {
                HR((*this)->CopyFromBitmap(nullptr,
                                           other.Get(),
                                           nullptr));
            }

            void CopyFromBitmap(Bitmap const & other,
                                Point2U const & destination) const
            {
                HR((*this)->CopyFromBitmap(destination.Get(),
                                           other.Get(),
                                           nullptr));
            }

            void CopyFromBitmap(Bitmap const & other,
                                RectU const & source) const
            {
                HR((*this)->CopyFromBitmap(nullptr,
                                           other.Get(),
                                           source.Get()));
            }

            void CopyFromBitmap(Bitmap const & other,
                                Point2U const & destination,
                                RectU const & source) const
            {
                HR((*this)->CopyFromBitmap(destination.Get(),
                                           other.Get(),
                                           source.Get()));
            }

            void CopyFromRenderTarget(RenderTarget const & other) const;

            void CopyFromRenderTarget(RenderTarget const & other,
                                      Point2U const & destination) const;

            void CopyFromRenderTarget(RenderTarget const & other,
                                      RectU const & source) const;

            void CopyFromRenderTarget(RenderTarget const & other,
                                      Point2U const & destination,
                                      RectU const & source) const;

            void CopyFromMemory(void const * data,
                                unsigned pitch) const
            {
                HR((*this)->CopyFromMemory(nullptr,
                                           data,
                                           pitch));
            }

            void CopyFromMemory(void const * data,
                                unsigned pitch,
                                RectU const & destination) const
            {
                HR((*this)->CopyFromMemory(destination.Get(),
                                           data,
                                           pitch));
            }
        };

        struct ColorContext : Resource
        {
            KENNYKERR_DEFINE_CLASS(ColorContext, Resource, ID2D1ColorContext)

            auto GetColorSpace() const -> ColorSpace
            {
                return static_cast<ColorSpace>((*this)->GetColorSpace());
            }

            auto GetProfileSize() const -> unsigned
            {
                return (*this)->GetProfileSize();
            }

            void GetProfile(BYTE * profile,
                            unsigned size) const
            {
                HR((*this)->GetProfile(profile,
                                       size));
            }
        };

        struct Bitmap1 : Bitmap
        {
            KENNYKERR_DEFINE_CLASS(Bitmap1, Bitmap, ID2D1Bitmap1)

            auto GetColorContext() const -> ColorContext
            {
                ColorContext result;
                (*this)->GetColorContext(result.GetAddressOf());
                return result;
            }

            auto GetOptions() const -> BitmapOptions
            {
                return static_cast<BitmapOptions>((*this)->GetOptions());
            }

            auto GetSurface() const -> Dxgi::Surface
            {
                Dxgi::Surface result;
                HR((*this)->GetSurface(result.GetAddressOf()));
                return result;
            }

            void Map(MapOptions options,
                     MappedRect & mappedRect) const
            {
                HR((*this)->Map(static_cast<D2D1_MAP_OPTIONS>(options),
                                mappedRect.Get()));
            }

            void Unmap() const
            {
                HR((*this)->Unmap());
            }
        };

        struct GradientStopCollection : Resource
        {
            KENNYKERR_DEFINE_CLASS(GradientStopCollection, Resource, ID2D1GradientStopCollection)

            auto GetGradientStopCount() const -> unsigned
            {
                return (*this)->GetGradientStopCount();
            }

            void GetGradientStops(GradientStop * stops,
                                  unsigned count) const
            {
                ASSERT(stops);
                ASSERT(count);

                (*this)->GetGradientStops(stops->Get(),
                                          count);
            }

            template <size_t Count>
            void GetGradientStops(GradientStop (&stops)[Count]) const
            {
                GetGradientStops(stops, Count);
            }

            auto GetColorInterpolationGamma() const -> Gamma
            {
                return static_cast<Gamma>((*this)->GetColorInterpolationGamma());
            }

            auto GetExtendMode() const -> ExtendMode
            {
                return static_cast<ExtendMode>((*this)->GetExtendMode());
            }
        };

        struct GradientStopCollection1 : GradientStopCollection
        {
            KENNYKERR_DEFINE_CLASS(GradientStopCollection1, GradientStopCollection, ID2D1GradientStopCollection1)

            void GetGradientStops1(GradientStop * stops,
                                  unsigned count) const
            {
                ASSERT(stops);
                ASSERT(count);

                (*this)->GetGradientStops1(stops->Get(),
                                          count);
            }

            template <size_t Count>
            void GetGradientStops1(GradientStop (&stops)[Count]) const
            {
                GetGradientStops1(stops, Count);
            }

            auto GetPreInterpolationSpace() const -> ColorSpace
            {
                return static_cast<ColorSpace>((*this)->GetPreInterpolationSpace());
            }

            auto GetPostInterpolationSpace() const -> ColorSpace
            {
                return static_cast<ColorSpace>((*this)->GetPostInterpolationSpace());
            }

            auto GetBufferPrecision() const -> BufferPrecision
            {
                return static_cast<BufferPrecision>((*this)->GetBufferPrecision());
            }

            auto GetColorInterpolationMode() const -> ColorInterpolationMode
            {
                return static_cast<ColorInterpolationMode>((*this)->GetColorInterpolationMode());
            }
        };

        struct Brush : Resource
        {
            KENNYKERR_DEFINE_CLASS(Brush, Resource, ID2D1Brush)

            void SetOpacity(float opacity) const
            {
                (*this)->SetOpacity(opacity);
            }

            float GetOpacity() const
            {
                return (*this)->GetOpacity();
            }

            void GetTransform(D2D1_MATRIX_3X2_F & transform) const
            {
                (*this)->GetTransform(&transform);
            }

            void SetTransform(D2D1_MATRIX_3X2_F const & transform) const
            {
                (*this)->SetTransform(transform);
            }
        };

        struct BitmapBrush : Brush
        {
            KENNYKERR_DEFINE_CLASS(BitmapBrush, Brush, ID2D1BitmapBrush)

            void SetExtendModeX(ExtendMode mode) const
            {
                (*this)->SetExtendModeX(static_cast<D2D1_EXTEND_MODE>(mode));
            }

            void SetExtendModeY(ExtendMode mode) const
            {
                (*this)->SetExtendModeY(static_cast<D2D1_EXTEND_MODE>(mode));
            }

            void SetInterpolationMode(BitmapInterpolationMode mode) const
            {
                (*this)->SetInterpolationMode(static_cast<D2D1_BITMAP_INTERPOLATION_MODE>(mode));
            }

            void SetBitmap(Bitmap const & bitmap) const
            {
                (*this)->SetBitmap(bitmap.Get());
            }

            auto GetExtendModeX() const -> ExtendMode
            {
                return static_cast<ExtendMode>((*this)->GetExtendModeX());
            }

            auto GetExtendModeY() const -> ExtendMode
            {
                return static_cast<ExtendMode>((*this)->GetExtendModeY());
            }

            auto GetInterpolationMode() const -> BitmapInterpolationMode
            {
                return static_cast<BitmapInterpolationMode>((*this)->GetInterpolationMode());
            }

            auto GetBitmap() const -> Bitmap
            {
                Bitmap result;
                (*this)->GetBitmap(result.GetAddressOf());
                return result;
            }
        };

        struct BitmapBrush1 : BitmapBrush
        {
            KENNYKERR_DEFINE_CLASS(BitmapBrush1, BitmapBrush, ID2D1BitmapBrush1)

            void SetInterpolationMode1(InterpolationMode mode) const
            {
                (*this)->SetInterpolationMode1(static_cast<D2D1_INTERPOLATION_MODE>(mode));
            }

            auto GetInterpolationMode1() const -> InterpolationMode
            {
                return static_cast<InterpolationMode>((*this)->GetInterpolationMode1());
            }
        };

        struct SolidColorBrush : Brush
        {
            KENNYKERR_DEFINE_CLASS(SolidColorBrush, Brush, ID2D1SolidColorBrush)

            void SetColor(Color const & color) const
            {
                (*this)->SetColor(color.Get());
            }

            auto GetColor() const -> Color
            {
                return (*this)->GetColor();
            }
        };

        struct LinearGradientBrush : Brush
        {
            KENNYKERR_DEFINE_CLASS(LinearGradientBrush, Brush, ID2D1LinearGradientBrush)

            void SetStartPoint(Point2F const & point) const
            {
                (*this)->SetStartPoint(point.Ref());
            }

            void SetEndPoint(Point2F const & point) const
            {
                (*this)->SetEndPoint(point.Ref());
            }

            auto GetStartPoint() const -> Point2F
            {
                return (*this)->GetStartPoint();
            }

            auto GetEndPoint() const -> Point2F
            {
                return (*this)->GetEndPoint();
            }

            auto GetGradientStopCollection() const -> GradientStopCollection
            {
                GradientStopCollection result;
                (*this)->GetGradientStopCollection(result.GetAddressOf());
                return result;
            };
        };

        struct RadialGradientBrush : Brush
        {
            KENNYKERR_DEFINE_CLASS(RadialGradientBrush, Brush, ID2D1RadialGradientBrush)

            void SetCenter(Point2F const & point) const
            {
                (*this)->SetCenter(point.Ref());
            }

            void SetGradientOriginOffset(Point2F const & point) const
            {
                (*this)->SetGradientOriginOffset(point.Ref());
            }

            void SetRadiusX(float radius) const
            {
                (*this)->SetRadiusX(radius);
            }

            void SetRadiusY(float radius) const
            {
                (*this)->SetRadiusY(radius);
            }

            auto GetCenter() const -> Point2F
            {
                return (*this)->GetCenter();
            }

            auto GetGradientOriginOffset() const -> Point2F
            {
                return (*this)->GetGradientOriginOffset();
            }

            auto GetRadiusX() const -> float
            {
                return (*this)->GetRadiusX();
            }

            auto GetRadiusY() const -> float
            {
                return (*this)->GetRadiusY();
            }

            auto GetGradientStopCollection() const -> GradientStopCollection
            {
                GradientStopCollection result;
                (*this)->GetGradientStopCollection(result.GetAddressOf());
                return result;
            }
        };

        struct StrokeStyle : Resource
        {
            KENNYKERR_DEFINE_CLASS(StrokeStyle, Resource, ID2D1StrokeStyle)

            auto GetStartCap() const -> CapStyle
            {
                return static_cast<CapStyle>((*this)->GetStartCap());
            }

            auto GetEndCap() const -> CapStyle
            {
                return static_cast<CapStyle>((*this)->GetEndCap());
            }
        
            auto GetDashCap() const -> CapStyle
            {
                return static_cast<CapStyle>((*this)->GetDashCap());
            }

            auto GetMiterLimit() const -> float
            {
                return (*this)->GetMiterLimit();
            }

            auto GetLineJoin() const -> LineJoin
            {
                return static_cast<LineJoin>((*this)->GetLineJoin());
            }

            auto GetDashOffset() const -> float
            {
                return (*this)->GetDashOffset();
            }

            auto GetDashStyle() const -> DashStyle
            {
                return static_cast<DashStyle>((*this)->GetDashStyle());
            }

            auto GetDashesCount() const -> unsigned
            {
                return (*this)->GetDashesCount();
            }

            void GetDashes(float * dashes,
                           unsigned count) const
            {
                (*this)->GetDashes(dashes,
                                   count);
            }

            template <size_t Count>
            void GetDashes(float (&dashes)[Count])
            {
                GetDashes(dashes,
                          Count);
            }
        };

        struct StrokeStyle1 : StrokeStyle
        {
            KENNYKERR_DEFINE_CLASS(StrokeStyle1, StrokeStyle, ID2D1StrokeStyle1)

            auto GetStrokeTransformType() const -> StrokeTransformType
            {
                static_cast<StrokeTransformType>((*this)->GetStrokeTransformType());
            }
        };

        struct Geometry : Resource
        {
            KENNYKERR_DEFINE_CLASS(Geometry, Resource, ID2D1Geometry)

            void GetBounds(RectF & bounds) const
            {
                HR((*this)->GetBounds(nullptr,
                                      bounds.Get()));
            }

            void GetBounds(RectF & bounds,
                           D2D1_MATRIX_3X2_F const & transform) const
            {
                HR((*this)->GetBounds(transform,
                                      bounds.Get()));
            }

            void GetWidenedBounds(RectF & bounds,
                                  float strokeWidth) const
            {
                HR((*this)->GetWidenedBounds(strokeWidth,
                                             nullptr,
                                             nullptr,
                                             bounds.Get()));
            }

            void GetWidenedBounds(RectF & bounds,
                                  float strokeWidth,
                                  float flatteningTolerance) const
            {
                HR((*this)->GetWidenedBounds(strokeWidth,
                                             nullptr,
                                             nullptr,
                                             flatteningTolerance,
                                             bounds.Get()));
            }

            void GetWidenedBounds(RectF & bounds,
                                  float strokeWidth,
                                  float flatteningTolerance,
                                  StrokeStyle const & strokeStyle,
                                  D2D1_MATRIX_3X2_F const & transform) const
            {
                HR((*this)->GetWidenedBounds(strokeWidth,
                                             strokeStyle.Get(),
                                             transform,
                                             flatteningTolerance,
                                             bounds.Get()));
            }

            bool StrokeContainsPoint(Point2F const & point,
                                     float strokeWidth) const
            {
                BOOL contains;

                HR((*this)->StrokeContainsPoint(point.Ref(),
                                                strokeWidth,
                                                nullptr,
                                                nullptr,
                                                &contains));

                return 0 != contains;
            }

            bool StrokeContainsPoint(Point2F const & point,
                                     float strokeWidth,
                                     float flatteningTolerance) const
            {
                BOOL contains;

                HR((*this)->StrokeContainsPoint(point.Ref(),
                                                strokeWidth,
                                                nullptr,
                                                nullptr,
                                                flatteningTolerance,
                                                &contains));

                return 0 != contains;
            }

            bool StrokeContainsPoint(Point2F const & point,
                                     float strokeWidth,
                                     float flatteningTolerance,
                                     StrokeStyle const & strokeStyle) const
            {
                BOOL contains;

                HR((*this)->StrokeContainsPoint(point.Ref(),
                                                strokeWidth,
                                                strokeStyle.Get(),
                                                nullptr,
                                                flatteningTolerance,
                                                &contains));

                return 0 != contains;
            }

            bool StrokeContainsPoint(Point2F const & point,
                                     float strokeWidth,
                                     float flatteningTolerance,
                                     StrokeStyle const & strokeStyle,
                                     D2D1_MATRIX_3X2_F const & transform) const
            {
                BOOL contains;

                HR((*this)->StrokeContainsPoint(point.Ref(),
                                                strokeWidth,
                                                strokeStyle.Get(),
                                                transform,
                                                flatteningTolerance,
                                                &contains));

                return 0 != contains;
            }

            bool FillContainsPoint(Point2F const & point) const
            {
                BOOL contains;

                HR((*this)->FillContainsPoint(point.Ref(),
                                              nullptr,
                                              &contains));

                return 0 != contains;
            }

            bool FillContainsPoint(Point2F const & point,
                                   float flatteningTolerance) const
            {
                BOOL contains;

                HR((*this)->FillContainsPoint(point.Ref(),
                                              nullptr,
                                              flatteningTolerance,
                                              &contains));

                return 0 != contains;
            }

            bool FillContainsPoint(Point2F const & point,
                                   float flatteningTolerance,
                                   D2D1_MATRIX_3X2_F const & transform) const
            {
                BOOL contains;

                HR((*this)->FillContainsPoint(point.Ref(),
                                              transform,
                                              flatteningTolerance,
                                              &contains));

                return 0 != contains;
            }

            auto CompareWithGeometry(Geometry const & other) const -> GeometryRelation
            {
                D2D1_GEOMETRY_RELATION result;

                HR((*this)->CompareWithGeometry(other.Get(),
                                                nullptr,
                                                &result));

                return static_cast<GeometryRelation>(result);
            }

            auto CompareWithGeometry(Geometry const & other,
                                     float flatteningTolerance) const -> GeometryRelation
            {
                D2D1_GEOMETRY_RELATION result;

                HR((*this)->CompareWithGeometry(other.Get(),
                                                nullptr,
                                                flatteningTolerance,
                                                &result));

                return static_cast<GeometryRelation>(result);
            }

            auto CompareWithGeometry(Geometry const & other,
                                     float flatteningTolerance,
                                     D2D1_MATRIX_3X2_F const & transform) const -> GeometryRelation
            {
                D2D1_GEOMETRY_RELATION result;

                HR((*this)->CompareWithGeometry(other.Get(),
                                                transform,
                                                flatteningTolerance,
                                                &result));

                return static_cast<GeometryRelation>(result);
            }

            void Simplify(GeometrySimplificationOption option,
                          SimplifiedGeometrySink const & sink) const
            {
                HR((*this)->Simplify(static_cast<D2D1_GEOMETRY_SIMPLIFICATION_OPTION>(option),
                                     nullptr,
                                     sink.Get()));
            }

            void Simplify(GeometrySimplificationOption option,
                          SimplifiedGeometrySink const & sink,
                          float flatteningTolerance) const
            {
                HR((*this)->Simplify(static_cast<D2D1_GEOMETRY_SIMPLIFICATION_OPTION>(option),
                                     nullptr,
                                     flatteningTolerance,
                                     sink.Get()));
            }

            void Simplify(GeometrySimplificationOption option,
                          SimplifiedGeometrySink const & sink,
                          float flatteningTolerance,
                          D2D1_MATRIX_3X2_F const & transform) const
            {
                HR((*this)->Simplify(static_cast<D2D1_GEOMETRY_SIMPLIFICATION_OPTION>(option),
                                     transform,
                                     flatteningTolerance,
                                     sink.Get()));
            }

            void Tessellate(TessellationSink const & sink) const
            {
                HR((*this)->Tessellate(nullptr,
                                       sink.Get()));
            }

            void Tessellate(TessellationSink const & sink,
                            float flatteningTolerance) const
            {
                HR((*this)->Tessellate(nullptr,
                                       flatteningTolerance,
                                       sink.Get()));
            }

            void Tessellate(TessellationSink const & sink,
                            float flatteningTolerance,
                            D2D1_MATRIX_3X2_F const & transform) const
            {
                HR((*this)->Tessellate(transform,
                                       flatteningTolerance,
                                       sink.Get()));
            }

            void CombineWithGeometry(Geometry const & other,
                                     CombineMode mode,
                                     SimplifiedGeometrySink const & sink) const
            {
                HR((*this)->CombineWithGeometry(other.Get(),
                                                static_cast<D2D1_COMBINE_MODE>(mode),
                                                nullptr,
                                                sink.Get()));
            }

            void CombineWithGeometry(Geometry const & other,
                                     CombineMode mode,
                                     SimplifiedGeometrySink const & sink,
                                     float flatteningTolerance) const
            {
                HR((*this)->CombineWithGeometry(other.Get(),
                                                static_cast<D2D1_COMBINE_MODE>(mode),
                                                nullptr,
                                                flatteningTolerance,
                                                sink.Get()));
            }

            void CombineWithGeometry(Geometry const & other,
                                     CombineMode mode,
                                     SimplifiedGeometrySink const & sink,
                                     float flatteningTolerance,
                                     D2D1_MATRIX_3X2_F const & transform) const
            {
                HR((*this)->CombineWithGeometry(other.Get(),
                                                static_cast<D2D1_COMBINE_MODE>(mode),
                                                transform,
                                                flatteningTolerance,
                                                sink.Get()));
            }

            void Outline(SimplifiedGeometrySink const & sink) const
            {
                HR((*this)->Outline(nullptr,
                                    sink.Get()));
            }

            void Outline(SimplifiedGeometrySink const & sink,
                         float flatteningTolerance) const
            {
                HR((*this)->Outline(nullptr,
                                    flatteningTolerance,
                                    sink.Get()));
            }

            void Outline(SimplifiedGeometrySink const & sink,
                         float flatteningTolerance,
                         D2D1_MATRIX_3X2_F const & transform) const
            {
                HR((*this)->Outline(transform,
                                    flatteningTolerance,
                                    sink.Get()));
            }

            auto ComputeArea() const -> float
            {
                float result;

                HR((*this)->ComputeArea(nullptr,
                                        &result));

                return result;
            }

            auto ComputeArea(float flatteningTolerance) const -> float
            {
                float result;

                HR((*this)->ComputeArea(nullptr,
                                        flatteningTolerance,
                                        &result));

                return result;
            }

            auto ComputeArea(float flatteningTolerance,
                             D2D1_MATRIX_3X2_F const & transform) const -> float
            {
                float result;

                HR((*this)->ComputeArea(transform,
                                        flatteningTolerance,
                                        &result));

                return result;
            }

            auto ComputeLength() const -> float
            {
                float result;

                HR((*this)->ComputeLength(nullptr,
                                          &result));

                return result;
            }

            auto ComputeLength(float flatteningTolerance) const -> float
            {
                float result;

                HR((*this)->ComputeLength(nullptr,
                                          flatteningTolerance,
                                          &result));

                return result;
            }

            auto ComputeLength(float flatteningTolerance,
                             D2D1_MATRIX_3X2_F const & transform) const -> float
            {
                float result;

                HR((*this)->ComputeLength(transform,
                                          flatteningTolerance,
                                          &result));

                return result;
            }

            auto ComputePointAtLength(float length) const -> Point2F
            {
                Point2F result;

                HR((*this)->ComputePointAtLength(length,
                                                 nullptr,
                                                 result.Get(),
                                                 nullptr));

                return result;
            }

            auto ComputePointAtLength(float length,
                                      float flatteningTolerance) const -> Point2F
            {
                Point2F result;

                HR((*this)->ComputePointAtLength(length,
                                                 nullptr,
                                                 flatteningTolerance,
                                                 result.Get(),
                                                 nullptr));

                return result;
            }

            auto ComputePointAtLength(float length,
                                      float flatteningTolerance,
                                      D2D1_MATRIX_3X2_F const & transform) const -> Point2F
            {
                Point2F result;

                HR((*this)->ComputePointAtLength(length,
                                                 transform,
                                                 flatteningTolerance,
                                                 result.Get(),
                                                 nullptr));

                return result;
            }

            auto ComputePointAtLength(Point2F & tangent,
                                      float length) const -> Point2F
            {
                Point2F result;

                HR((*this)->ComputePointAtLength(length,
                                                 nullptr,
                                                 result.Get(),
                                                 tangent.Get()));

                return result;
            }

            auto ComputePointAtLength(Point2F & tangent,
                                      float length,
                                      float flatteningTolerance) const -> Point2F
            {
                Point2F result;

                HR((*this)->ComputePointAtLength(length,
                                                 nullptr,
                                                 flatteningTolerance,
                                                 result.Get(),
                                                 tangent.Get()));

                return result;
            }

            auto ComputePointAtLength(Point2F & tangent,
                                      float length,
                                      float flatteningTolerance,
                                      D2D1_MATRIX_3X2_F const & transform) const -> Point2F
            {
                Point2F result;

                HR((*this)->ComputePointAtLength(length,
                                                 transform,
                                                 flatteningTolerance,
                                                 result.Get(),
                                                 tangent.Get()));

                return result;
            }

            void Widen(float strokeWidth,
                       SimplifiedGeometrySink const & sink) const
            {
                HR((*this)->Widen(strokeWidth,
                                  nullptr,
                                  nullptr,
                                  sink.Get()));
            }

            void Widen(float strokeWidth,
                       SimplifiedGeometrySink const & sink,
                       float flatteningTolerance) const
            {
                HR((*this)->Widen(strokeWidth,
                                  nullptr,
                                  nullptr,
                                  flatteningTolerance,
                                  sink.Get()));
            }

            void Widen(float strokeWidth,
                       SimplifiedGeometrySink const & sink,
                       float flatteningTolerance,
                       StrokeStyle const & strokeStyle) const
            {
                HR((*this)->Widen(strokeWidth,
                                  strokeStyle.Get(),
                                  nullptr,
                                  flatteningTolerance,
                                  sink.Get()));
            }

            void Widen(float strokeWidth,
                       SimplifiedGeometrySink const & sink,
                       float flatteningTolerance,
                       StrokeStyle const & strokeStyle,
                       D2D1_MATRIX_3X2_F const & transform) const
            {
                HR((*this)->Widen(strokeWidth,
                                  strokeStyle.Get(),
                                  transform,
                                  flatteningTolerance,
                                  sink.Get()));
            }
        };

        struct RectangleGeometry : Geometry
        {
            KENNYKERR_DEFINE_CLASS(RectangleGeometry, Geometry, ID2D1RectangleGeometry)

            void GetRect(RectF & rect) const
            {
                (*this)->GetRect(rect.Get());
            } 
        };

        struct RoundedRectangleGeometry : Geometry
        {
            KENNYKERR_DEFINE_CLASS(RoundedRectangleGeometry, Geometry, ID2D1RoundedRectangleGeometry)

            void GetRoundedRect(RoundedRect & rect) const
            {
                (*this)->GetRoundedRect(rect.Get());
            } 
        };

        struct EllipseGeometry : Geometry
        {
            KENNYKERR_DEFINE_CLASS(EllipseGeometry, Geometry, ID2D1EllipseGeometry)

            void GetEllipse(Ellipse & ellipse) const
            {
                (*this)->GetEllipse(ellipse.Get());
            } 
        };

        struct GeometryGroup : Geometry
        {
            KENNYKERR_DEFINE_CLASS(GeometryGroup, Geometry, ID2D1GeometryGroup)

            auto GetFillMode() const -> FillMode
            {
                return static_cast<FillMode>((*this)->GetFillMode());
            }

            auto GetSourceGeometryCount() const -> unsigned
            {
                return (*this)->GetSourceGeometryCount();
            }

            // TODO: GetSourceGeometries
        };

        struct TransformedGeometry : Geometry
        {
            KENNYKERR_DEFINE_CLASS(TransformedGeometry, Geometry, ID2D1TransformedGeometry)

            auto GetSourceGeometry() const -> Geometry
            {
                Geometry result;
                (*this)->GetSourceGeometry(result.GetAddressOf());
                return result;
            }

            void GetTransform(D2D1_MATRIX_3X2_F & transform) const
            {
                (*this)->GetTransform(&transform);
            }
        };

        struct GeometrySink : SimplifiedGeometrySink
        {
            KENNYKERR_DEFINE_CLASS(GeometrySink, SimplifiedGeometrySink, ID2D1GeometrySink)

            void AddLine(Point2F const & point) const
            {
                (*this)->AddLine(point.Ref());
            }

            void AddBezier(BezierSegment const & bezier) const
            {
                (*this)->AddBezier(bezier.Ref());
            }

            void AddQuadraticBezier(QuadraticBezierSegment const & bezier) const
            {
                (*this)->AddQuadraticBezier(bezier.Ref());
            }

            void AddArc(ArcSegment const & arc) const
            {
                (*this)->AddArc(arc.Ref());
            }

            void AddQuadraticBeziers(QuadraticBezierSegment const * beziers,
                                     unsigned count) const
            {
                ASSERT(beziers);
                ASSERT(count);

                (*this)->AddQuadraticBeziers(beziers->Get(),
                                             count);
            }

            template <size_t Count>
            void AddQuadraticBeziers(QuadraticBezierSegment const (&beziers),
                                     unsigned count) const
            {
                AddQuadraticBeziers(beziers,
                                    Count);
            }
        };

        struct PathGeometry : Geometry
        {
            KENNYKERR_DEFINE_CLASS(PathGeometry, Geometry, ID2D1PathGeometry)

            auto Open() const -> GeometrySink
            {
                GeometrySink result;
                HR((*this)->Open(result.GetAddressOf()));
                return result;
            }

            void Stream(GeometrySink const & sink) const
            {
                HR((*this)->Stream(sink.Get()));
            }

            auto GetSegmentCount() const -> unsigned
            {
                unsigned result;
                HR((*this)->GetSegmentCount(&result));
                return result;
            }

            auto GetFigureCount() const -> unsigned
            {
                unsigned result;
                HR((*this)->GetFigureCount(&result));
                return result;
            }
        };

        struct PathGeometry1 : PathGeometry
        {
            KENNYKERR_DEFINE_CLASS(PathGeometry1, PathGeometry, ID2D1PathGeometry1)

            void ComputePointAndSegmentAtLength(float length,
                                                unsigned startSegment,
                                                PointDescription & description,
                                                float flatteningTolerance = D2D1_DEFAULT_FLATTENING_TOLERANCE) const
            {
                HR((*this)->ComputePointAndSegmentAtLength(length,
                                                           startSegment,
                                                           nullptr,
                                                           flatteningTolerance,
                                                           description.Get()));
            }

            void ComputePointAndSegmentAtLength(float length,
                                                unsigned startSegment,
                                                PointDescription & description,
                                                float flatteningTolerance,
                                                D2D1_MATRIX_3X2_F const & transform) const
            {
                HR((*this)->ComputePointAndSegmentAtLength(length,
                                                           startSegment,
                                                           transform,
                                                           flatteningTolerance,
                                                           description.Get()));
            }
        };

        struct Properties : Details::Object
        {
            KENNYKERR_DEFINE_CLASS(Properties, Details::Object, ID2D1Properties)

            // TODO: ...
        };

        struct Effect : Properties
        {
            KENNYKERR_DEFINE_CLASS(Effect, Properties, ID2D1Effect)

            void SetInput(unsigned index,
                          bool invalidate = true) const
            {
                (*this)->SetInput(index,
                                  nullptr,
                                  invalidate);
            }

            void SetInput(unsigned index,
                          Image const & input,
                          bool invalidate = true) const
            {
                (*this)->SetInput(index,
                                  input.Get(),
                                  invalidate);
            }

            void SetInput(Image const & input,
                          bool invalidate = true) const
            {
                SetInput(0,
                         input,
                         invalidate);
            }

            void SetInputCount(unsigned count) const
            {
                HR((*this)->SetInputCount(count));
            }

            auto GetInput(unsigned index) const -> Image
            {
                Image result;

                (*this)->GetInput(index,
                                  result.GetAddressOf());

                return result;
            }

            auto GetInputCount() const -> unsigned
            {
                return (*this)->GetInputCount();
            }

            auto GetOutput() const -> Image
            {
                Image result;
                (*this)->GetOutput(result.GetAddressOf());
                return result;
            }

            void SetInputEffect(unsigned index,
                                Effect const & input,
                                bool invalidate = true)
            {
                Image output = input.GetOutput();

                if (output)
                {
                    SetInput(index, output, invalidate);
                }
                else
                {
                    SetInput(index, invalidate);
                }
            }
        };

        struct Mesh : Resource
        {
            KENNYKERR_DEFINE_CLASS(Mesh, Resource, ID2D1Mesh)

            auto Open() const -> TessellationSink
            {
                TessellationSink result;
                HR((*this)->Open(result.GetAddressOf()));
                return result;
            }
        };

        struct Layer : Resource
        {
            KENNYKERR_DEFINE_CLASS(Layer, Resource, ID2D1Layer)

            auto GetSize() const -> SizeF
            {
                return (*this)->GetSize();
            }
        };

        struct DrawingStateBlock : Resource
        {
            KENNYKERR_DEFINE_CLASS(DrawingStateBlock, Resource, ID2D1DrawingStateBlock)

            void GetDescription(DrawingStateDescription & description) const
            {
                (*this)->GetDescription(description.Get());
            }

            void SetDescription(DrawingStateDescription const & description) const
            {
                (*this)->SetDescription(description.Get());
            }

            void SetTextRenderingParams() const
            {
                (*this)->SetTextRenderingParams();
            }

            void SetTextRenderingParams(DirectWrite::RenderingParams const & params) const
            {
                (*this)->SetTextRenderingParams(params.Get());
            }

            auto GetTextRenderingParams() const -> DirectWrite::RenderingParams
            {
                DirectWrite::RenderingParams result;
                (*this)->GetTextRenderingParams(result.GetAddressOf());
                return result;
            }
        };

        struct DrawingStateBlock1 : DrawingStateBlock
        {
            KENNYKERR_DEFINE_CLASS(DrawingStateBlock1, DrawingStateBlock, ID2D1DrawingStateBlock1)

            void GetDescription(DrawingStateDescription1 & description) const
            {
                (*this)->GetDescription(description.Get());
            }

            void SetDescription(DrawingStateDescription1 const & description) const
            {
                (*this)->SetDescription(description.Get());
            }
        };

        struct RenderTarget : Resource
        {
            KENNYKERR_DEFINE_CLASS(RenderTarget, Resource, ID2D1RenderTarget)

            auto CreateBitmap(SizeU const & size,
                              void const * data,
                              unsigned pitch,
                              BitmapProperties const & properties) const -> Bitmap
            {
                Bitmap result;

                HR((*this)->CreateBitmap(size.Ref(),
                                         data,
                                         pitch,
                                         reinterpret_cast<D2D1_BITMAP_PROPERTIES const *>(properties.Get()),
                                         result.GetAddressOf()));

                return result;
            }

            auto CreateBitmap(SizeU const & size,
                              BitmapProperties const & properties) const -> Bitmap
            {
                return CreateBitmap(size,
                                    nullptr, 0, // not initialized
                                    properties);
            }

            auto CreateBitmapFromWicBitmap(Wic::BitmapSource const & source) const -> Bitmap
            {
                Bitmap result;

                HR((*this)->CreateBitmapFromWicBitmap(source.Get(),
                                                      result.GetAddressOf()));

                return result;
            }

            auto CreateBitmapFromWicBitmap(Wic::BitmapSource const & source,
                                           BitmapProperties const & properties) const -> Bitmap
            {
                Bitmap result;

                HR((*this)->CreateBitmapFromWicBitmap(source.Get(),
                                                      reinterpret_cast<D2D1_BITMAP_PROPERTIES const *>(properties.Get()),
                                                      result.GetAddressOf()));

                return result;
            }

            template <typename T>
            auto CreateSharedBitmap(T const & source) const -> Bitmap
            {
                Bitmap result;

                HR((*this)->CreateSharedBitmap(__uuidof(T),
                                               source.Get(),
                                               nullptr,
                                               result.GetAddressOf()));

                return Result;
            }

            template <typename T>
            auto CreateSharedBitmap(T const & source,
                                    BitmapProperties const & properties) const -> Bitmap
            {
                Bitmap result;

                HR((*this)->CreateSharedBitmap(__uuidof(T),
                                               source.Get(),
                                               reinterpret_cast<D2D1_BITMAP_PROPERTIES const *>(properties.Get()),
                                               result.GetAddressOf()));

                return Result;
            }

            auto CreateBitmapBrush(Bitmap const & bitmap) const -> BitmapBrush
            {
                BitmapBrush result;

                HR((*this)->CreateBitmapBrush(bitmap.Get(),
                                              result.GetAddressOf()));

                return result;
            }

            auto CreateBitmapBrush(Bitmap const & bitmap,
                                   BitmapBrushProperties const & bitmapBrushProperties) const -> BitmapBrush
            {
                BitmapBrush result;

                HR((*this)->CreateBitmapBrush(bitmap.Get(),
                                              bitmapBrushProperties.Ref(),
                                              result.GetAddressOf()));

                return result;
            }

            auto CreateBitmapBrush(Bitmap const & bitmap,
                                   BitmapBrushProperties const & bitmapBrushProperties,
                                   BrushProperties const & brushProperties) const -> BitmapBrush
            {
                BitmapBrush result;

                HR((*this)->CreateBitmapBrush(bitmap.Get(),
                                              bitmapBrushProperties.Ref(),
                                              brushProperties.Ref(),
                                              result.GetAddressOf()));

                return result;
            }

            auto CreateSolidColorBrush(Color const & color,
                                       BrushProperties const &  properties) const -> SolidColorBrush
            {
                SolidColorBrush result;

                HR((*this)->CreateSolidColorBrush(color.Get(),
                                                  properties.Get(),
                                                  result.GetAddressOf()));

                return result;
            }

            auto CreateSolidColorBrush(Color const & color) const -> SolidColorBrush
            {
                SolidColorBrush result;

                HR((*this)->CreateSolidColorBrush(*color.Get(),
                                                  result.GetAddressOf()));

                return result;
            }

            auto CreateGradientStopCollection(GradientStop const * stops,
                                              unsigned count,
                                              Gamma gamma = Gamma::_2_2,
                                              ExtendMode mode = ExtendMode::Clamp) const -> GradientStopCollection
            {
                GradientStopCollection result;

                HR((*this)->CreateGradientStopCollection(stops->Get(),
                                                         count,
                                                         static_cast<D2D1_GAMMA>(gamma),
                                                         static_cast<D2D1_EXTEND_MODE>(mode),
                                                         result.GetAddressOf()));

                return result;
            }

            template <size_t Count>
            auto CreateGradientStopCollection(GradientStop const (&stops)[Count],
                                              Gamma gamma = Gamma::_2_2,
                                              ExtendMode mode = ExtendMode::Clamp) const -> GradientStopCollection
            {
                return CreateGradientStopCollection(stops,
                                                    Count,
                                                    gamma,
                                                    mode);
            }

            auto CreateLinearGradientBrush(GradientStopCollection const & stops,
                                           LinearGradientBrushProperties const & linearGradientBrushProperties = LinearGradientBrushProperties()) const -> LinearGradientBrush
            {
                LinearGradientBrush result;

                HR((*this)->CreateLinearGradientBrush(linearGradientBrushProperties.Ref(),
                                                      stops.Get(),
                                                      result.GetAddressOf()));

                return result;
            }

            auto CreateLinearGradientBrush(GradientStopCollection const & stops,
                                           LinearGradientBrushProperties const & linearGradientBrushProperties,
                                           BrushProperties const &  brushProperties) const -> LinearGradientBrush
            {
                LinearGradientBrush result;

                HR((*this)->CreateLinearGradientBrush(linearGradientBrushProperties.Ref(),
                                                      brushProperties.Ref(),
                                                      stops.Get(),
                                                      result.GetAddressOf()));

                return result;
            }

            auto CreateRadialGradientBrush(RadialGradientBrushProperties const & radialGradientBrushProperties,
                                           GradientStopCollection const & stops) const -> RadialGradientBrush
            {
                RadialGradientBrush result;

                HR((*this)->CreateRadialGradientBrush(radialGradientBrushProperties.Ref(),
                                                      stops.Get(),
                                                      result.GetAddressOf()));

                return result;
            }

            auto CreateRadialGradientBrush(RadialGradientBrushProperties const & radialGradientBrushProperties,
                                           BrushProperties const &  brushProperties,
                                           GradientStopCollection const & stops) const -> RadialGradientBrush
            {
                RadialGradientBrush result;

                HR((*this)->CreateRadialGradientBrush(radialGradientBrushProperties.Ref(),
                                                      brushProperties.Ref(),
                                                      stops.Get(),
                                                      result.GetAddressOf()));

                return result;
            }

            auto CreateCompatibleRenderTarget() const -> BitmapRenderTarget;
            auto CreateCompatibleRenderTarget(SizeF const & size) const -> BitmapRenderTarget;

            auto CreateCompatibleRenderTarget(SizeF const & size,
                                              SizeU const & pixelSize) const -> BitmapRenderTarget;

            auto CreateCompatibleRenderTarget(SizeF const & size,
                                              SizeU const & pixelSize,
                                              PixelFormat const & format) const -> BitmapRenderTarget;

            auto CreateCompatibleRenderTarget(SizeF const & size,
                                              SizeU const & pixelSize,
                                              PixelFormat const & format,
                                              CompatibleRenderTargetOptions options) const -> BitmapRenderTarget;

            auto CreateLayer() const -> Layer
            {
                Layer result;
                HR((*this)->CreateLayer(result.GetAddressOf()));
                return result;
            }

            auto CreateLayer(SizeF const & size) const -> Layer
            {
                Layer result;

                HR((*this)->CreateLayer(size.Ref(),
                                        result.GetAddressOf()));

                return result;
            }

            auto CreatMesh() const -> Mesh
            {
                Mesh result;
                HR((*this)->CreateMesh(result.GetAddressOf()));
                return result;
            }

            void DrawLine(Point2F const & point0,
                          Point2F const & point1,
                          Brush const & brush,
                          float strokeWidth = 1.0f) const
            {
                (*this)->DrawLine(point0.Ref(),
                                  point1.Ref(),
                                  brush.Get(),
                                  strokeWidth);
            }

            void DrawLine(Point2F const & point0,
                          Point2F const & point1,
                          Brush const & brush,
                          float strokeWidth,
                          StrokeStyle const & strokeStyle) const
            {
                (*this)->DrawLine(point0.Ref(),
                                  point1.Ref(),
                                  brush.Get(),
                                  strokeWidth,
                                  strokeStyle.Get());
            }

            void DrawRectangle(RectF const & rect,
                               Brush const & brush,
                               float strokeWidth = 1.0f) const
            {
                (*this)->DrawRectangle(rect.Ref(),
                                       brush.Get(),
                                       strokeWidth);
            }

            void DrawRectangle(RectF const & rect,
                               Brush const & brush,
                               float strokeWidth,
                               StrokeStyle const & strokeStyle) const
            {
                (*this)->DrawRectangle(rect.Ref(),
                                       brush.Get(),
                                       strokeWidth,
                                       strokeStyle.Get());
            }

            void FillRectangle(RectF const & rect,
                               Brush const & brush) const
            {
                (*this)->FillRectangle(rect.Get(),
                                       brush.Get());
            }

            void DrawRoundedRectangle(RoundedRect const & rect,
                                      Brush const & brush,
                                      float strokeWidth = 1.0f) const
            {
                (*this)->DrawRoundedRectangle(rect.Ref(),
                                              brush.Get(),
                                              strokeWidth);
            }

            void DrawRoundedRectangle(RoundedRect const & rect,
                                      Brush const & brush,
                                      float strokeWidth,
                                      StrokeStyle const & strokeStyle) const
            {
                (*this)->DrawRoundedRectangle(rect.Ref(),
                                              brush.Get(),
                                              strokeWidth,
                                              strokeStyle.Get());
            }

            void FillRoundedRectangle(RoundedRect const & rect,
                                      Brush const & brush)
            {
                (*this)->FillRoundedRectangle(rect.Get(),
                                              brush.Get());
            }

            void DrawEllipse(Ellipse const & ellipse,
                             Brush const & brush,
                             float strokeWidth = 1.0f) const
            {
                (*this)->DrawEllipse(ellipse.Ref(),
                                     brush.Get(),
                                     strokeWidth);
            }

            void DrawEllipse(Ellipse const & ellipse,
                             Brush const & brush,
                             float strokeWidth,
                             StrokeStyle const & strokeStyle) const
            {
                (*this)->DrawEllipse(ellipse.Ref(),
                                     brush.Get(),
                                     strokeWidth,
                                     strokeStyle.Get());
            }

            void FillEllipse(Ellipse const & ellipse,
                                      Brush const & brush) const
            {
                (*this)->FillEllipse(ellipse.Get(),
                                     brush.Get());
            }

            void DrawGeometry(Geometry const & geometry,
                              Brush const & brush,
                              float strokeWidth = 1.0f) const
            {
                (*this)->DrawGeometry(geometry.Get(),
                                      brush.Get(),
                                      strokeWidth);
            }

            void DrawGeometry(Geometry const & geometry,
                              Brush const & brush,
                              float strokeWidth,
                              StrokeStyle const & strokeStyle) const
            {
                (*this)->DrawGeometry(geometry.Get(),
                                      brush.Get(),
                                      strokeWidth,
                                      strokeStyle.Get());
            }

            void FillGeometry(Geometry const & geometry,
                              Brush const & brush) const
            {
                (*this)->FillGeometry(geometry.Get(),
                                      brush.Get());
            }

            void FillGeometry(Geometry const & geometry,
                              Brush const & brush,
                              Brush const & opacityBrush) const
            {
                (*this)->FillGeometry(geometry.Get(),
                                      brush.Get(),
                                      opacityBrush.Get());
            }

            void FillMesh(Mesh const & mesh,
                          Brush const & brush) const
            {
                (*this)->FillMesh(mesh.Get(),
                                  brush.Get());
            }

            void FillOpacityMask(Bitmap const & mask,
                                 Brush const & brush,
                                 OpacityMaskContent content) const
            {
                (*this)->FillOpacityMask(mask.Get(),
                                         brush.Get(),
                                         static_cast<D2D1_OPACITY_MASK_CONTENT>(content));
            }

            void FillOpacityMask(Bitmap const & mask,
                                 Brush const & brush,
                                 OpacityMaskContent content,
                                 RectF const & destination,
                                 RectF const & source) const
            {
                (*this)->FillOpacityMask(mask.Get(),
                                         brush.Get(),
                                         static_cast<D2D1_OPACITY_MASK_CONTENT>(content),
                                         destination.Get(),
                                         source.Get());
            }

            void DrawBitmap(Bitmap const & bitmap,
                            float opacity = 1.0f) const
            {
                (*this)->DrawBitmap(bitmap.Get(),
                                    nullptr,
                                    opacity);
            }

            void DrawBitmap(Bitmap const & bitmap,
                            RectF const & destination,
                            float opacity = 1.0f,
                            InterpolationMode interpolationMode = InterpolationMode::Linear) const
            {
                (*this)->DrawBitmap(bitmap.Get(),
                                    destination.Get(),
                                    opacity,
                                    static_cast<D2D1_BITMAP_INTERPOLATION_MODE>(interpolationMode));
            }

            void DrawBitmap(Bitmap const & bitmap,
                            RectF const & destination,
                            RectF const & source,
                            float opacity = 1.0f,
                            InterpolationMode interpolationMode = InterpolationMode::Linear) const
            {
                (*this)->DrawBitmap(bitmap.Get(),
                                    destination.Get(),
                                    opacity,
                                    static_cast<D2D1_BITMAP_INTERPOLATION_MODE>(interpolationMode),
                                    source.Get());
            }

            void DrawText(wchar_t const * string,
                          unsigned length,
                          DirectWrite::TextFormat const & textFormat,
                          RectF const & layoutRect,
                          Brush const & brush,
                          DrawTextOptions options = DrawTextOptions::None,
                          DirectWrite::MeasuringMode measuringMode = DirectWrite::MeasuringMode::Natural) const
            {
                (*this)->DrawText(string,
                                  length,
                                  textFormat.Get(),
                                  layoutRect.Get(),
                                  brush.Get(),
                                  static_cast<D2D1_DRAW_TEXT_OPTIONS>(options),
                                  static_cast<DWRITE_MEASURING_MODE>(measuringMode));
            }

            void DrawTextLayout(Point2F const & origin,
                                DirectWrite::TextLayout const & textLayout,
                                Brush const & brush,
                                DrawTextOptions options = DrawTextOptions::None) const
            {
                (*this)->DrawTextLayout(origin.Ref(),
                                        textLayout.Get(),
                                        brush.Get(),
                                        static_cast<D2D1_DRAW_TEXT_OPTIONS>(options));
            }

            // TODO: DrawGlyphRun

            void SetTransform(D2D1_MATRIX_3X2_F const & transform) const
            {
                (*this)->SetTransform(transform);
            }

            void GetTransform(D2D1_MATRIX_3X2_F & transform) const
            {
                (*this)->GetTransform(&transform);
            }

            void SetAntialiasMode(AntialiasMode mode) const
            {
                (*this)->SetAntialiasMode(static_cast<D2D1_ANTIALIAS_MODE>(mode));
            }

            auto GetAntialiasMode() const -> AntialiasMode
            {
                static_cast<D2D1_ANTIALIAS_MODE>((*this)->GetAntialiasMode());
            }

            void SetTextAntialiasMode(TextAntialiasMode mode) const
            {
                (*this)->SetTextAntialiasMode(static_cast<D2D1_TEXT_ANTIALIAS_MODE>(mode));
            }

            auto GetTextAntialiasMode() const -> TextAntialiasMode
            {
                static_cast<D2D1_TEXT_ANTIALIAS_MODE>((*this)->GetTextAntialiasMode());
            }

            void SetTextRenderingParams() const
            {
                (*this)->SetTextRenderingParams();
            }

            void SetTextRenderingParams(DirectWrite::RenderingParams const & params) const
            {
                (*this)->SetTextRenderingParams(params.Get());
            }

            auto GetTextRenderingParams() const -> DirectWrite::RenderingParams
            {
                DirectWrite::RenderingParams result;
                (*this)->GetTextRenderingParams(result.GetAddressOf());
                return result;
            }

            void SetTags(UINT64 tag1, UINT64 tag2) const
            {
                (*this)->SetTags(tag1, tag2);
            }

            void GetTags(UINT64 & tag1, UINT64 & tag2) const
            {
                (*this)->GetTags(&tag1, &tag2);
            }

            void PushLayer(LayerProperties const & properties) const
            {
                (*this)->PushLayer(properties.Get(),
                                   nullptr);
            }

            void PushLayer(LayerProperties const & properties,
                           Layer const & layer) const
            {
                (*this)->PushLayer(properties.Get(),
                                   layer.Get());
            }

            void PopLayer() const
            {
                (*this)->PopLayer();
            }

            void Flush() const
            {
                HR((*this)->Flush());
            }

            void Flush(UINT64 & tag1, UINT64 & tag2) const
            {
                HR((*this)->Flush(&tag1, &tag2));
            }

            void SaveDrawingState(DrawingStateBlock const & block) const
            {
                (*this)->SaveDrawingState(block.Get());
            }

            void RestoreDrawingState(DrawingStateBlock const & block) const
            {
                (*this)->RestoreDrawingState(block.Get());
            }

            void PushAxisAlignedClip(RectF const & rect,
                                     AntialiasMode mode) const
            {
                (*this)->PushAxisAlignedClip(rect.Get(),
                                             static_cast<D2D1_ANTIALIAS_MODE>(mode));
            }

            void PopAxisAlignedClip() const
            {
                (*this)->PopAxisAlignedClip();
            }

            void Clear() const
            {
                (*this)->Clear();
            }

            void Clear(Color const & color) const
            {
                (*this)->Clear(color.Get());
            }

            void BeginDraw() const
            {
                (*this)->BeginDraw();
            }

            auto EndDraw() const -> HRESULT
            {
                return (*this)->EndDraw();
            }

            auto EndDraw(UINT64 & tag1, UINT64 & tag2) const -> HRESULT
            {
                return (*this)->EndDraw(&tag1, &tag2);
            }

            auto GetPixelFormat() const -> PixelFormat
            {
                return (*this)->GetPixelFormat();
            }

            void SetDpi(float x, float y) const
            {
                (*this)->SetDpi(x, y);
            }

            void GetDpi(float & x, float & y) const
            {
                (*this)->GetDpi(&x, &y);
            }

            auto GetSize() const -> SizeF
            {
                return (*this)->GetSize();
            }

            auto GetPixelSize() const -> SizeU
            {
                return (*this)->GetPixelSize();
            }

            auto GetMaximumBitmapSize() const -> unsigned
            {
                return (*this)->GetMaximumBitmapSize();
            }

            auto IsSupported(RenderTargetProperties const & properties) const -> bool
            {
                return 0 != (*this)->IsSupported(properties.Get());
            }
        };

        struct BitmapRenderTarget : RenderTarget
        {
            KENNYKERR_DEFINE_CLASS(BitmapRenderTarget, RenderTarget, ID2D1BitmapRenderTarget)

            auto GetBitmap() const -> Bitmap
            {
                Bitmap result;
                HR((*this)->GetBitmap(result.GetAddressOf()));
                return result;
            }
        };

        struct HwndRenderTarget : RenderTarget
        {
            KENNYKERR_DEFINE_CLASS(HwndRenderTarget, RenderTarget, ID2D1HwndRenderTarget)

            auto CheckWindowState() const -> WindowState
            {
                return static_cast<WindowState>((*this)->CheckWindowState());
            }

            auto Resize(SizeU const & size) const -> HRESULT
            {
                return (*this)->Resize(size.Get());
            }

            auto GetHwnd() const -> HWND
            {
                return (*this)->GetHwnd();
            }
        };

        #if WINAPI_FAMILY_DESKTOP_APP == WINAPI_FAMILY
        struct GdiInteropRenderTarget : Details::Object
        {
            KENNYKERR_DEFINE_CLASS(GdiInteropRenderTarget, Details::Object, ID2D1GdiInteropRenderTarget)

            auto GetDC(DcInitializeMode mode) const -> HDC
            {
                HDC dc;
                HR((*this)->GetDC(static_cast<D2D1_DC_INITIALIZE_MODE>(mode), &dc));
                return dc;
            }

            void ReleaseDC() const
            {
                HR((*this)->ReleaseDC(nullptr));
            }

            void ReleaseDC(RECT const & rect) const
            {
                HR((*this)->ReleaseDC(&rect));
            }
        };
        #endif

        struct DcRenderTarget : RenderTarget
        {
            KENNYKERR_DEFINE_CLASS(DcRenderTarget, RenderTarget, ID2D1DCRenderTarget)

            void BindDC(HDC dc,
                        RECT const & rect) const
            {
                HR((*this)->BindDC(dc, &rect));
            }
        };

        struct GdiMetafileSink : Details::Object
        {
            KENNYKERR_DEFINE_CLASS(GdiMetafileSink, Details::Object, ID2D1GdiMetafileSink)
        };

        struct GdiMetafile : Resource
        {
            KENNYKERR_DEFINE_CLASS(GdiMetafile, Resource, ID2D1GdiMetafile)

            void Stream(GdiMetafileSink const & sink) const
            {
                HR((*this)->Stream(sink.Get()));
            }

            void GetBounds(RectF & rect) const
            {
                HR((*this)->GetBounds(rect.Get()));
            }
        };

        struct CommandSink : Details::Object
        {
            KENNYKERR_DEFINE_CLASS(CommandSink, Details::Object, ID2D1CommandSink)
        };

        struct CommandList : Image
        {
            KENNYKERR_DEFINE_CLASS(CommandList, Image, ID2D1CommandList)

            void Stream(CommandSink const & sink) const
            {
                HR((*this)->Stream(sink.Get()));
            }

            void Close() const
            {
                HR((*this)->Close());
            }
        };

        struct PrintControl : Details::Object
        {
            KENNYKERR_DEFINE_CLASS(PrintControl, Details::Object, ID2D1PrintControl)

            void AddPage(CommandList const & commandList,
                         SizeF const & pageSize) const
            {
                HR((*this)->AddPage(commandList.Get(),
                                    pageSize.Ref(),
                                    nullptr));
            }

            // TODO: AddPage overloads

            void Close() const
            {
                HR((*this)->Close());
            }
        };

        struct ImageBrush : Brush
        {
            KENNYKERR_DEFINE_CLASS(ImageBrush, Brush, ID2D1ImageBrush)

            void SetImage() const
            {
                (*this)->SetImage(nullptr);
            }

            void SetImage(Image const & image) const
            {
                (*this)->SetImage(image.Get());
            }

            void SetExtendModeX(ExtendMode mode) const
            {
                (*this)->SetExtendModeX(static_cast<D2D1_EXTEND_MODE>(mode));
            }

            void SetExtendModeY(ExtendMode mode) const
            {
                (*this)->SetExtendModeY(static_cast<D2D1_EXTEND_MODE>(mode));
            }

            void SetInterpolationMode(InterpolationMode mode) const
            {
                (*this)->SetInterpolationMode(static_cast<D2D1_INTERPOLATION_MODE>(mode));
            }

            void SetSourceRectangle(RectF const & rect) const
            {
                (*this)->SetSourceRectangle(rect.Get());
            }

            auto GetImage() const -> Image
            {
                Image result;
                (*this)->GetImage(result.GetAddressOf());
                return result;
            }

            auto GetExtendModeX() const -> ExtendMode
            {
                return static_cast<ExtendMode>((*this)->GetExtendModeX());
            }

            auto GetExtendModeY() const -> ExtendMode
            {
                return static_cast<ExtendMode>((*this)->GetExtendModeY());
            }

            auto GetInterpolationMode() const -> InterpolationMode
            {
                return static_cast<InterpolationMode>((*this)->GetInterpolationMode());
            }

            void GetSourceRectangle(RectF & rect) const
            {
                (*this)->GetSourceRectangle(rect.Get());
            }
        };

        struct DeviceContext : RenderTarget
        {
            KENNYKERR_DEFINE_CLASS(DeviceContext, RenderTarget, ID2D1DeviceContext)

            auto CreateBitmap(SizeU const & size,
                              void const * data,
                              unsigned pitch,
                              BitmapProperties1 const & properties) const -> Bitmap1
            {
                Bitmap1 result;

                HR((*this)->CreateBitmap(size.Ref(),
                                         data,
                                         pitch,
                                         properties.Get(),
                                         result.GetAddressOf()));

                return result;
            }

            auto CreateBitmap(SizeU const & size,
                              BitmapProperties1 const & properties) const -> Bitmap1
            {
                return CreateBitmap(size,
                                    nullptr, 0, // not initialized
                                    properties);
            }

            auto CreateBitmapFromWicBitmap(Wic::BitmapSource const & source) const -> Bitmap1
            {
                Bitmap1 result;

                HR((*this)->CreateBitmapFromWicBitmap(source.Get(),
                                                      result.GetAddressOf()));

                return result;
            }

            auto CreateBitmapFromWicBitmap(Wic::BitmapSource const & source,
                                           BitmapProperties1 const & properties) const -> Bitmap1
            {
                Bitmap1 result;

                HR((*this)->CreateBitmapFromWicBitmap(source.Get(),
                                                      properties.Get(),
                                                      result.GetAddressOf()));

                return result;
            }

            auto CreateColorContext(ColorSpace space,
                                    BYTE const * profile,
                                    unsigned size) const -> ColorContext
            {
                ColorContext result;

                HR((*this)->CreateColorContext(static_cast<D2D1_COLOR_SPACE>(space),
                                               profile,
                                               size,
                                               result.GetAddressOf()));

                return result;
            }

            auto CreateColorContextFromFilename(PCWSTR filename) const -> ColorContext
            {
                ColorContext result;

                HR((*this)->CreateColorContextFromFilename(filename,
                                                           result.GetAddressOf()));

                return result;
            }

            auto CreateColorContextFromWicColorContext(Wic::ColorContext const & source) const -> ColorContext
            {
                ColorContext result;

                HR((*this)->CreateColorContextFromWicColorContext(source.Get(),
                                                                  result.GetAddressOf()));

                return result;
            }

            auto CreateBitmapFromDxgiSurface(Dxgi::Surface const & surface,
                                             BitmapProperties1 const & properties) const -> Bitmap1
            {
                Bitmap1 result;

                HR((*this)->CreateBitmapFromDxgiSurface(surface.Get(),
                                                        properties.Get(),
                                                        result.GetAddressOf()));

                return result;
            }

            auto CreateEffect(REFCLSID clsid) const -> Effect
            {
                Effect result;

                HR((*this)->CreateEffect(clsid,
                                         result.GetAddressOf()));

                return result;
            }

            auto CreateShadowEffect() const -> Effect
            {
                struct __declspec(uuid("C67EA361-1863-4e69-89DB-695D3E9A5B6B")) Class;

                return CreateEffect(__uuidof(Class));
            }


            auto CreateGradientStopCollection(GradientStop const * stops,
                                              unsigned count,
                                              ColorSpace preInterpolationSpace,
                                              ColorSpace postInterpolationSpace,
                                              BufferPrecision bufferPrecision,
                                              ExtendMode extendMode,
                                              ColorInterpolationMode colorInterpolationMode) const -> GradientStopCollection1
            {
                GradientStopCollection1 result;

                HR((*this)->CreateGradientStopCollection(stops->Get(),
                                                         count,
                                                         static_cast<D2D1_COLOR_SPACE>(preInterpolationSpace),
                                                         static_cast<D2D1_COLOR_SPACE>(postInterpolationSpace),
                                                         static_cast<D2D1_BUFFER_PRECISION>(bufferPrecision),
                                                         static_cast<D2D1_EXTEND_MODE>(extendMode),
                                                         static_cast<D2D1_COLOR_INTERPOLATION_MODE>(colorInterpolationMode),
                                                         result.GetAddressOf()));

                return result;
            }

            template <size_t Count>
            auto CreateGradientStopCollection(GradientStop const (&stops)[Count],
                                              ColorSpace preInterpolationSpace,
                                              ColorSpace postInterpolationSpace,
                                              BufferPrecision bufferPrecision,
                                              ExtendMode extendMode,
                                              ColorInterpolationMode colorInterpolationMode) const -> GradientStopCollection1
            {
                return CreateGradientStopCollection(stops,
                                                    Count,
                                                    preInterpolationSpace,
                                                    postInterpolationSpace,
                                                    bufferPrecision,
                                                    extendMode,
                                                    colorInterpolationMode);
            }

            auto CreateImageBrush(Image const & image,
                                  ImageBrushProperties const & imageBrushProperties) const -> ImageBrush
            {
                ImageBrush result;

                HR((*this)->CreateImageBrush(image.Get(),
                                             imageBrushProperties.Get(),
                                             nullptr,
                                             result.GetAddressOf()));

                return result;
            }

            auto CreateImageBrush(Image const & image,
                                  ImageBrushProperties const & imageBrushProperties,
                                  BrushProperties const & brushProperties) const -> ImageBrush
            {
                ImageBrush result;

                HR((*this)->CreateImageBrush(image.Get(),
                                             imageBrushProperties.Get(),
                                             brushProperties.Get(),
                                             result.GetAddressOf()));

                return result;
            }

            auto CreateBitmapBrush(Bitmap const & bitmap) const -> BitmapBrush1
            {
                BitmapBrush1 result;

                HR((*this)->CreateBitmapBrush(bitmap.Get(),
                                              nullptr,
                                              nullptr,
                                              result.GetAddressOf()));

                return result;
            }

            auto CreateBitmapBrush(Bitmap const & bitmap,
                                   BitmapBrushProperties1 const & bitmapBrushProperties) const -> BitmapBrush1
            {
                BitmapBrush1 result;

                HR((*this)->CreateBitmapBrush(bitmap.Get(),
                                              bitmapBrushProperties.Get(),
                                              nullptr,
                                              result.GetAddressOf()));

                return result;
            }

            auto CreateBitmapBrush(Bitmap const & bitmap,
                                   BitmapBrushProperties1 const & bitmapBrushProperties,
                                   BrushProperties const & brushProperties) const -> BitmapBrush1
            {
                BitmapBrush1 result;

                HR((*this)->CreateBitmapBrush(bitmap.Get(),
                                              bitmapBrushProperties.Get(),
                                              brushProperties.Get(),
                                              result.GetAddressOf()));

                return result;
            }

            auto CreateCommandList() const -> CommandList
            {
                CommandList result;
                HR((*this)->CreateCommandList(result.GetAddressOf()));
                return result;
            }

            auto IsDxgiFormatSupported(Dxgi::Format format) const -> bool
            {
                return 0 != (*this)->IsDxgiFormatSupported(static_cast<DXGI_FORMAT>(format));
            }

            auto IsBufferPrecisionSupported(BufferPrecision precision) const -> bool
            {
                return 0 != (*this)->IsBufferPrecisionSupported(static_cast<D2D1_BUFFER_PRECISION>(precision));
            }

            void GetImageLocalBounds(Image const & image,
                                     RectF & bounds) const
            {
                HR((*this)->GetImageLocalBounds(image.Get(),
                                                bounds.Get()));
            }

            void GetImageWorldBounds(Image const & image,
                                     RectF & bounds) const
            {
                HR((*this)->GetImageWorldBounds(image.Get(),
                                                bounds.Get()));
            }

            // TODO: GetGlyphRunWorldBounds

            auto GetDevice() const -> Device;

            void SetTarget(Image const & image) const
            {
                (*this)->SetTarget(image.Get());
            }

            void SetTarget() const
            {
                (*this)->SetTarget(nullptr);
            }

            auto GetTarget() const -> Image
            {
                Image result;
                (*this)->GetTarget(result.GetAddressOf());
                return result;
            }

            void SetRenderingControls(RenderingControls const & controls) const
            {
                (*this)->SetRenderingControls(controls.Get());
            }

            void GetRenderingControls(RenderingControls & controls) const
            {
                (*this)->GetRenderingControls(controls.Get());
            }

            void SetPrimitiveBlend(PrimitiveBlend blend) const
            {
                (*this)->SetPrimitiveBlend(static_cast<D2D1_PRIMITIVE_BLEND>(blend));
            }

            auto GetPrimitiveBlend() const -> PrimitiveBlend
            {
                return static_cast<PrimitiveBlend>((*this)->GetPrimitiveBlend());
            }

            void SetUnitMode(UnitMode mode) const
            {
                (*this)->SetUnitMode(static_cast<D2D1_UNIT_MODE>(mode));
            }

            auto GetUnitMode() const -> UnitMode
            {
                return static_cast<UnitMode>((*this)->GetUnitMode());
            }

            // TODO: DrawGlyphRun

            void DrawImage(Image const & image,
                           InterpolationMode interpolationMode = InterpolationMode::Linear,
                           CompositeMode compositeMode = CompositeMode::SourceOver) const
            {
                (*this)->DrawImage(image.Get(),
                                   static_cast<D2D1_INTERPOLATION_MODE>(interpolationMode),
                                   static_cast<D2D1_COMPOSITE_MODE>(compositeMode));
            }

            void DrawImage(Effect const & effect,
                           InterpolationMode interpolationMode = InterpolationMode::Linear,
                           CompositeMode compositeMode = CompositeMode::SourceOver) const
            {
                (*this)->DrawImage(effect.GetOutput().Get(),
                                   static_cast<D2D1_INTERPOLATION_MODE>(interpolationMode),
                                   static_cast<D2D1_COMPOSITE_MODE>(compositeMode));
            }

            void DrawImage(Image const & image,
                           Point2F const & targetOffset,
                           RectF const & imageRectangle,
                           InterpolationMode interpolationMode = InterpolationMode::Linear,
                           CompositeMode compositeMode = CompositeMode::SourceOver) const
            {
                (*this)->DrawImage(image.Get(),
                                   targetOffset.Get(),
                                   imageRectangle.Get(),
                                   static_cast<D2D1_INTERPOLATION_MODE>(interpolationMode),
                                   static_cast<D2D1_COMPOSITE_MODE>(compositeMode));
            }

            void DrawImage(Effect const & effect,
                           Point2F const & targetOffset,
                           RectF const & imageRectangle,
                           InterpolationMode interpolationMode = InterpolationMode::Linear,
                           CompositeMode compositeMode = CompositeMode::SourceOver) const
            {
                (*this)->DrawImage(effect.GetOutput().Get(),
                                   targetOffset.Get(),
                                   imageRectangle.Get(),
                                   static_cast<D2D1_INTERPOLATION_MODE>(interpolationMode),
                                   static_cast<D2D1_COMPOSITE_MODE>(compositeMode));
            }

            void DrawGdiMetafile(GdiMetafile const & metafile) const
            {
                (*this)->DrawGdiMetafile(metafile.Get());
            }

            void DrawGdiMetafile(GdiMetafile const & metafile,
                                 Point2F const & targetOffset) const
            {
                (*this)->DrawGdiMetafile(metafile.Get(),
                                         targetOffset.Get());
            }

            // TODO: DrawBitmap

            // TODO: PushLayer

            void InvalidateEffectInputRectangle(Effect const & effect,
                                                unsigned input,
                                                RectF const & rect) const
            {
                HR((*this)->InvalidateEffectInputRectangle(effect.Get(),
                                                           input,
                                                           rect.Get()));
            }

            auto GetEffectInvalidRectangleCount(Effect const & effect) -> unsigned
            {
                unsigned result;

                HR((*this)->GetEffectInvalidRectangleCount(effect.Get(),
                                                           &result));

                return result;
            }

            void GetEffectInvalidRectangles(Effect const & effect,
                                            RectF * rectangles,
                                            unsigned count) const
            {
                HR((*this)->GetEffectInvalidRectangles(effect.Get(),
                                                       rectangles->Get(),
                                                       count));
            }

            // TODO: GetEffectRequiredInputRectangles

            void FillOpacityMask(Bitmap const & opacityMask,
                                 Brush const & brush) const
            {
                (*this)->FillOpacityMask(opacityMask.Get(),
                                         brush.Get());
            }

            void FillOpacityMask(Bitmap const & opacityMask,
                                 Brush const & brush,
                                 RectF const & destinationRectangle) const
            {
                (*this)->FillOpacityMask(opacityMask.Get(),
                                         brush.Get(),
                                         destinationRectangle.Get());
            }

            void FillOpacityMask(Bitmap const & opacityMask,
                                 Brush const & brush,
                                 RectF const & destinationRectangle,
                                 RectF const & sourceRectangle) const
            {
                (*this)->FillOpacityMask(opacityMask.Get(),
                                         brush.Get(),
                                         destinationRectangle.Get(),
                                         sourceRectangle.Get());
            }
        };

        struct Device : Resource
        {
            KENNYKERR_DEFINE_CLASS(Device, Resource, ID2D1Device)

            auto CreateDeviceContext(DeviceContextOptions options = DeviceContextOptions::None) const -> DeviceContext
            {
                DeviceContext result;

                HR((*this)->CreateDeviceContext(static_cast<D2D1_DEVICE_CONTEXT_OPTIONS>(options),
                                                result.GetAddressOf()));

                return result;
            }

            // CreatePrintControl

            void SetMaximumTextureMemory(UINT64 maximumInBytes) const
            {
                (*this)->SetMaximumTextureMemory(maximumInBytes);
            }

            auto GetMaximumTextureMemory() const -> UINT64
            {
                return (*this)->GetMaximumTextureMemory();
            }

            void ClearResources(unsigned millisecondsSinceUse = 0) const
            {
                (*this)->ClearResources(millisecondsSinceUse);
            }
        };

        struct MultiThread : Details::Object
        {
            KENNYKERR_DEFINE_CLASS(MultiThread, Details::Object, ID2D1Multithread)

            auto GetProtected() const -> bool
            {
                return 0 != (*this)->GetMultithreadProtected();
            }

            void Enter() const
            {
                (*this)->Enter();
            }

            void Leave() const
            {
                (*this)->Leave();
            }
        };

        struct Factory : Details::Object
        {
            KENNYKERR_DEFINE_CLASS(Factory, Details::Object, ID2D1Factory)

            auto AsMultiThread() const -> MultiThread
            {
                MultiThread result;
                HR(m_ptr.CopyTo(result.GetAddressOf()));
                return result;
            }

            auto GetDesktopDpi() const -> float
            {
                float x, y;
                (*this)->GetDesktopDpi(&x, &y);
                return x;
            }

            auto CreateRectangleGeometry(RectF const & rect) const -> RectangleGeometry
            {
                RectangleGeometry result;

                HR((*this)->CreateRectangleGeometry(rect.Get(),
                                                    result.GetAddressOf()));

                return result;
            }

            auto CreateRoundedRectangleGeometry(RoundedRect const & roundedRect) const -> RoundedRectangleGeometry
            {
                RoundedRectangleGeometry result;

                HR((*this)->CreateRoundedRectangleGeometry(roundedRect.Get(),
                                                           result.GetAddressOf()));

                return result;
            }

            auto CreateEllipseGeometry(Ellipse const & ellipse) const -> EllipseGeometry
            {
                EllipseGeometry result;

                HR((*this)->CreateEllipseGeometry(ellipse.Get(),
                                                  result.GetAddressOf()));

                return result;
            }

            auto CreateGeometryGroup(FillMode mode,
                                     ID2D1Geometry ** geometries,
                                     unsigned count) const -> GeometryGroup
            {
                GeometryGroup result;

                HR((*this)->CreateGeometryGroup(static_cast<D2D1_FILL_MODE>(mode),
                                                geometries,
                                                count,
                                                result.GetAddressOf()));

                return result;
            }

            template <size_t Count>
            auto CreateGeometryGroup(FillMode mode,
                                     ID2D1Geometry * (&geometries)[Count],
                                     unsigned count) const -> GeometryGroup
            {
                return CreateGeometryGroup(mode,
                                           geometries,
                                           Count);
            }

            auto CreateTransformedGeometry(Geometry const & source,
                                           D2D1_MATRIX_3X2_F const & transform) -> TransformedGeometry
            {
                TransformedGeometry result;

                HR((*this)->CreateTransformedGeometry(source.Get(),
                                                      transform,
                                                      result.GetAddressOf()));

                return result;
            }

            auto CreatePathGeometry() const -> PathGeometry
            {
                PathGeometry result;
                HR((*this)->CreatePathGeometry(result.GetAddressOf()));
                return result;
            }

            auto CreateStrokeStyle(StrokeStyleProperties const & properties) const -> StrokeStyle
            {
                StrokeStyle result;

                HR((*this)->CreateStrokeStyle(reinterpret_cast<D2D1_STROKE_STYLE_PROPERTIES const *>(properties.Get()),
                                              nullptr, 0, // no custom dashes
                                              result.GetAddressOf()));

                return result;
            }

            auto CreateStrokeStyle(StrokeStyleProperties const & properties,
                                   float const * dashes,
                                   unsigned count) const -> StrokeStyle
            {
                ASSERT(properties.DashStyle == DashStyle::Custom);

                StrokeStyle result;

                HR((*this)->CreateStrokeStyle(reinterpret_cast<D2D1_STROKE_STYLE_PROPERTIES const *>(properties.Get()),
                                              dashes,
                                              count,
                                              result.GetAddressOf()));

                return result;
            }

            template <size_t Count>
            auto CreateStrokeStyle(StrokeStyleProperties const & properties,
                                   float const (&dashes)[Count]) const -> StrokeStyle
            {
                return CreateStrokeStyle(properties,
                                         dashes,
                                         Count);
            }

            auto CreateDrawingStateBlock() const -> DrawingStateBlock
            {
                DrawingStateBlock result;
                HR((*this)->CreateDrawingStateBlock(result.GetAddressOf()));
                return result;
            }

            auto CreateDrawingStateBlock(DrawingStateDescription const & description) const -> DrawingStateBlock
            {
                DrawingStateBlock result;

                HR((*this)->CreateDrawingStateBlock(description.Ref(),
                                                    result.GetAddressOf()));

                return result;
            }

            auto CreateDrawingStateBlock(DrawingStateDescription const & description,
                                         DirectWrite::RenderingParams const & params) const -> DrawingStateBlock
            {
                DrawingStateBlock result;

                HR((*this)->CreateDrawingStateBlock(description.Get(),
                                                    params.Get(),
                                                    result.GetAddressOf()));

                return result;
            }

            auto CreateWicBitmapRenderTarget(Wic::Bitmap const & target,
                                             RenderTargetProperties const & properties) const -> RenderTarget
            {
                RenderTarget result;

                HR((*this)->CreateWicBitmapRenderTarget(target.Get(),
                                                        properties.Get(),
                                                        result.GetAddressOf()));

                return result;
            }

            auto CreateHwndRenderTarget(RenderTargetProperties const & renderTargetProperties,
                                        HwndRenderTargetProperties const & hwndRenderTargetProperties) const -> HwndRenderTarget
            {
                HwndRenderTarget result;

                HR((*this)->CreateHwndRenderTarget(renderTargetProperties.Get(),
                                                   hwndRenderTargetProperties.Get(),
                                                   result.GetAddressOf()));

                return result;
            }

            auto CreateDxgiSurfaceRenderTarget(Dxgi::Surface const & surface,
                                               RenderTargetProperties const & renderTargetProperties) const -> RenderTarget
            {
                RenderTarget result;

                HR((*this)->CreateDxgiSurfaceRenderTarget(surface.Get(),
                                                          renderTargetProperties.Get(),
                                                          result.GetAddressOf()));

                return result;
            }

            auto CreateDcRenderTarget(RenderTargetProperties const & renderTargetProperties) const -> DcRenderTarget
            {
                DcRenderTarget result;

                HR((*this)->CreateDCRenderTarget(renderTargetProperties.Get(),
                                                 result.GetAddressOf()));

                return result;
            }
        };

        struct Factory1 : Factory
        {
            KENNYKERR_DEFINE_CLASS(Factory1, Factory, ID2D1Factory1)

            auto CreateDevice(Dxgi::Device const & device) const -> Device
            {
                Device result;

                HR((*this)->CreateDevice(device.Get(),
                                         result.GetAddressOf()));

                return result;
            }

            auto CreateDevice(Direct3D::Device const & device) const -> Device
            {
                return CreateDevice(device.AsDxgi());
            }

            // TODO: remaining methods
        };

        inline auto Resource::GetFactory() const -> Factory
        {
            Factory result;
            (*this)->GetFactory(result.GetAddressOf());
            return result;
        }

        inline void Bitmap::CopyFromRenderTarget(RenderTarget const & other) const
        {
            HR((*this)->CopyFromRenderTarget(nullptr,
                                             other.Get(),
                                             nullptr));
        }

        inline void Bitmap::CopyFromRenderTarget(RenderTarget const & other,
                                                 Point2U const & destination) const
        {
            HR((*this)->CopyFromRenderTarget(destination.Get(),
                                             other.Get(),
                                             nullptr));
        }

        inline void Bitmap::CopyFromRenderTarget(RenderTarget const & other,
                                                 RectU const & source) const
        {
            HR((*this)->CopyFromRenderTarget(nullptr,
                                             other.Get(),
                                             source.Get()));
        }

        inline void Bitmap::CopyFromRenderTarget(RenderTarget const & other,
                                                 Point2U const & destination,
                                                 RectU const & source) const
        {
            HR((*this)->CopyFromRenderTarget(destination.Get(),
                                             other.Get(),
                                             source.Get()));
        }

        inline auto RenderTarget::CreateCompatibleRenderTarget() const -> BitmapRenderTarget
        {
            BitmapRenderTarget result;
            HR((*this)->CreateCompatibleRenderTarget(result.GetAddressOf()));
            return result;
        }

        inline auto RenderTarget::CreateCompatibleRenderTarget(SizeF const & size) const -> BitmapRenderTarget
        {
            BitmapRenderTarget result;

            HR((*this)->CreateCompatibleRenderTarget(size.Ref(),
                                                     result.GetAddressOf()));

            return result;
        }

        inline auto RenderTarget::CreateCompatibleRenderTarget(SizeF const & size,
                                                               SizeU const & pixelSize) const -> BitmapRenderTarget
        {
            BitmapRenderTarget result;

            HR((*this)->CreateCompatibleRenderTarget(size.Ref(),
                                                     pixelSize.Ref(),
                                                     result.GetAddressOf()));

            return result;
        }

        inline auto RenderTarget::CreateCompatibleRenderTarget(SizeF const & size,
                                                               SizeU const & pixelSize,
                                                               PixelFormat const & format) const -> BitmapRenderTarget
        {
            BitmapRenderTarget result;

            HR((*this)->CreateCompatibleRenderTarget(size.Ref(),
                                                     pixelSize.Ref(),
                                                     format.Ref(),
                                                     result.GetAddressOf()));

            return result;
        }

        inline auto RenderTarget::CreateCompatibleRenderTarget(SizeF const & size,
                                                               SizeU const & pixelSize,
                                                               PixelFormat const & format,
                                                               CompatibleRenderTargetOptions options) const -> BitmapRenderTarget
        {
            BitmapRenderTarget result;

            HR((*this)->CreateCompatibleRenderTarget(size.Ref(),
                                                     pixelSize.Ref(),
                                                     format.Ref(),
                                                     static_cast<D2D1_COMPATIBLE_RENDER_TARGET_OPTIONS>(options),
                                                     result.GetAddressOf()));

            return result;
        }

        inline auto DeviceContext::GetDevice() const -> Device
        {
            Device result;
            (*this)->GetDevice(result.GetAddressOf());
            return result;
        }

        #pragma endregion

        #pragma region Functions

        inline auto CreateFactory(FactoryType mode = FactoryType::SingleThreaded) -> Factory1
        {
            Factory1 result;

            #ifdef _DEBUG
            D2D1_FACTORY_OPTIONS options = { D2D1_DEBUG_LEVEL_INFORMATION };
            #else
            D2D1_FACTORY_OPTIONS options = { D2D1_DEBUG_LEVEL_NONE };
            #endif

            HR(D2D1CreateFactory(static_cast<D2D1_FACTORY_TYPE>(mode),
                                 options,
                                 result.GetAddressOf()));

            return result;
        }

        #pragma endregion
    }
}
