#pragma once

#include <windows.h>
#include <d2d1_1.h>
#include <d3d11_1.h>
#include <dwrite_1.h>
#include <wincodec.h>
#include <uianimation.h>
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

#ifndef VERIFY
#ifdef _DEBUG
#define VERIFY(expression) ASSERT(expression)
#else
#define VERIFY(expression) (expression)
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
    OutputDebugStringW(output);
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
            Object(IUnknown * other) : m_ptr(other) {}
            Object(Object const & other) : m_ptr(other.m_ptr) {}
            Object(Object && other) : m_ptr(std::move(other.m_ptr)) {}
            void Copy(Object const & other) { m_ptr = other.m_ptr; }
            void Copy(IUnknown * other) { m_ptr = other; }
            void Move(Object && other) { m_ptr = std::move(other.m_ptr); }

        public:

            operator Details::BoolType() const { return nullptr != m_ptr ? &Details::BoolStruct::Member : nullptr; }
            auto Unknown() const -> IUnknown * { return m_ptr.Get(); };
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
        THIS_CLASS(INTERFACE * other)        : BASE_CLASS(other) {}                                                                            \
        THIS_CLASS & operator=(THIS_CLASS const & other) { Copy(other);            return *this; }                                             \
        THIS_CLASS & operator=(THIS_CLASS && other)      { Move(std::move(other)); return *this; }                                             \
        THIS_CLASS & operator=(INTERFACE * other)        { Copy(other);            return *this; }                                             \
        Details::RemoveAddRefRelease<INTERFACE> * operator->() const { return static_cast<Details::RemoveAddRefRelease<INTERFACE> *>(Get()); } \
        auto Get() const -> INTERFACE *     {                 return static_cast<INTERFACE *>(m_ptr.Get()); }                                  \
        auto GetAddressOf() -> INTERFACE ** { ASSERT(!m_ptr); return reinterpret_cast<INTERFACE **>(m_ptr.GetAddressOf()); }

        // Would love to use a static_assert here but that can't appear inside the class definition.
        #define KENNYKERR_DEFINE_STRUCT(THIS_STRUCT, BASE_STRUCT)                                        \
        THIS_STRUCT(BASE_STRUCT const & other) { *this = reinterpret_cast<THIS_STRUCT const &>(other); } \
        auto Get() const -> BASE_STRUCT const * { ASSERT(sizeof(THIS_STRUCT) == sizeof(BASE_STRUCT));    \
                                                  return reinterpret_cast<BASE_STRUCT const *>(this); }; \
        auto Get()       -> BASE_STRUCT *       { ASSERT(sizeof(THIS_STRUCT) == sizeof(BASE_STRUCT));    \
                                                  return reinterpret_cast<BASE_STRUCT *>(this);       }; \
        auto Ref() const -> BASE_STRUCT const & { return *Get();                                      }; \
        auto Ref()       -> BASE_STRUCT &       { return *Get();                                      };
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

    #pragma region Enumerations

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
            None                         = 0,
            NonPrerotated                = DXGI_SWAP_CHAIN_FLAG_NONPREROTATED,
            AllowModeSwitch              = DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH,
            GdiCompatible                = DXGI_SWAP_CHAIN_FLAG_GDI_COMPATIBLE,
            RestrictedContent            = DXGI_SWAP_CHAIN_FLAG_RESTRICTED_CONTENT,
            RestrictSharedResourceDriver = DXGI_SWAP_CHAIN_FLAG_RESTRICT_SHARED_RESOURCE_DRIVER,
            DisplayOnly                  = DXGI_SWAP_CHAIN_FLAG_DISPLAY_ONLY,
        };
        DEFINE_ENUM_FLAG_OPERATORS(SwapChainFlag)

        enum class Present
        {
            None            = 0,
            Test                = DXGI_PRESENT_TEST,
            DoNotSequence       = DXGI_PRESENT_DO_NOT_SEQUENCE,
            Restart             = DXGI_PRESENT_RESTART,
            DoNotWait           = DXGI_PRESENT_DO_NOT_WAIT,
            StereoPreferRight   = DXGI_PRESENT_STEREO_PREFER_RIGHT,
            StereoTemporaryMono = DXGI_PRESENT_STEREO_TEMPORARY_MONO,
            RestrictToOutput    = DXGI_PRESENT_RESTRICT_TO_OUTPUT,
        };
        DEFINE_ENUM_FLAG_OPERATORS(Present)

    } // Dxgi

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

        enum class Usage
        {
            Default   = D3D11_USAGE_DEFAULT,
            Immutable = D3D11_USAGE_IMMUTABLE,
            Dynamic   = D3D11_USAGE_DYNAMIC,
            Staging   = D3D11_USAGE_STAGING,
        };

        enum class BindFlag
        {
            None            = 0,
            VertexBuffer    = D3D11_BIND_VERTEX_BUFFER,
            IndexBuffer     = D3D11_BIND_INDEX_BUFFER,
            ConstantBuffer  = D3D11_BIND_CONSTANT_BUFFER,
            ShaderResource  = D3D11_BIND_SHADER_RESOURCE,
            StreamOutput    = D3D11_BIND_STREAM_OUTPUT,
            RenderTarget    = D3D11_BIND_RENDER_TARGET,
            DepthStencil    = D3D11_BIND_DEPTH_STENCIL,
            UnorderedAccess = D3D11_BIND_UNORDERED_ACCESS,
            Decoder         = D3D11_BIND_DECODER,
            VideoEncoder    = D3D11_BIND_VIDEO_ENCODER,
        };
        DEFINE_ENUM_FLAG_OPERATORS(BindFlag)

        enum class CpuAccessFlag
        {
            None  = 0,
            Write = D3D11_CPU_ACCESS_WRITE,
            Read  = D3D11_CPU_ACCESS_READ,
        };
        DEFINE_ENUM_FLAG_OPERATORS(CpuAccessFlag)

        enum class ResourceMiscFlag
        {
            None                         = 0,
            GenerateMips                 = D3D11_RESOURCE_MISC_GENERATE_MIPS,
            Shared                       = D3D11_RESOURCE_MISC_SHARED,
            TextureCube                  = D3D11_RESOURCE_MISC_TEXTURECUBE,
            DrawIndirectArgs             = D3D11_RESOURCE_MISC_DRAWINDIRECT_ARGS,
            BufferAllowRawViews          = D3D11_RESOURCE_MISC_BUFFER_ALLOW_RAW_VIEWS,
            BufferStructured             = D3D11_RESOURCE_MISC_BUFFER_STRUCTURED,
            ResourceClamp                = D3D11_RESOURCE_MISC_RESOURCE_CLAMP,
            SharedKeyedMutex             = D3D11_RESOURCE_MISC_SHARED_KEYEDMUTEX,
            GdiCompatible                = D3D11_RESOURCE_MISC_GDI_COMPATIBLE,
            SharedNtHandle               = D3D11_RESOURCE_MISC_SHARED_NTHANDLE,
            RestrictedContent            = D3D11_RESOURCE_MISC_RESTRICTED_CONTENT,
            RestrictSharedResource       = D3D11_RESOURCE_MISC_RESTRICT_SHARED_RESOURCE,
            RestrictSharedResourceDriver = D3D11_RESOURCE_MISC_RESTRICT_SHARED_RESOURCE_DRIVER,
            Guarded                      = D3D11_RESOURCE_MISC_GUARDED,
        };
        DEFINE_ENUM_FLAG_OPERATORS(ResourceMiscFlag)

    } // Direct3D

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

        enum class DecodeCacheOption
        {
            OnDemand = WICDecodeMetadataCacheOnDemand,
            OnLoad   = WICDecodeMetadataCacheOnLoad,
        };

    } // Wic

    namespace Wam
    {
    } // Wam

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

    } // DirectWrite

    namespace Direct2D
    {
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
            None           = D2D1_PRESENT_OPTIONS_NONE,
            RetainContents = D2D1_PRESENT_OPTIONS_RETAIN_CONTENTS,
            Immediately    = D2D1_PRESENT_OPTIONS_IMMEDIATELY,
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

    } // Direct2D

    #pragma endregion Enumerations

    #pragma region Structures

    struct SizeU
    {
        KENNYKERR_DEFINE_STRUCT(SizeU, D2D1_SIZE_U)

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
        KENNYKERR_DEFINE_STRUCT(SizeF, D2D1_SIZE_F)

        explicit SizeF(float const width  = 0.0f,
                       float const height = 0.0f) :
            Width(width),
            Height(height)
        {}

        float Width;
        float Height;
    };

    struct Point2F
    {
        KENNYKERR_DEFINE_STRUCT(Point2F, D2D1_POINT_2F)

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
        KENNYKERR_DEFINE_STRUCT(Point2U, D2D1_POINT_2U)

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
        KENNYKERR_DEFINE_STRUCT(RectF, D2D1_RECT_F)

        static RectF Infinite()
        {
            return RectF(-D2D1::FloatMax(), -D2D1::FloatMax(), D2D1::FloatMax(),  D2D1::FloatMax());
        }

        explicit RectF(float const left   = 0.0f,
                       float const top    = 0.0f,
                       float const right  = 0.0f,
                       float const bottom = 0.0f) :
            Left(left),
            Top(top),
            Right(right),
            Bottom(bottom)
        {}

        auto Width() const -> float
        {
            return Right - Left;
        }

        auto Height() const -> float
        {
            return Bottom - Top;
        }

        float Left;
        float Top;
        float Right;
        float Bottom;
    };

    struct RectU
    {
        KENNYKERR_DEFINE_STRUCT(RectU, D2D1_RECT_U)

        explicit RectU(unsigned const left   = 0,
                       unsigned const top    = 0,
                       unsigned const right  = 0,
                       unsigned const bottom = 0) :
            Left(left),
            Top(top),
            Right(right),
            Bottom(bottom)
        {}

        auto Width() const -> unsigned
        {
            return Right - Left;
        }

        auto Height() const -> unsigned
        {
            return Bottom - Top;
        }

        unsigned Left;
        unsigned Top;
        unsigned Right;
        unsigned Bottom;
    };

    struct Color
    {
        KENNYKERR_DEFINE_STRUCT(Color, D2D1_COLOR_F)

        explicit Color(float const red   = 0.0f,
                       float const green = 0.0f,
                       float const blue  = 0.0f,
                       float const alpha = 1.0f) :
            Red(red),
            Green(green),
            Blue(blue),
            Alpha(alpha)
        {}

        float Red;
        float Green;
        float Blue;
        float Alpha;
    };

    namespace Dxgi
    {
        struct SampleDescription
        {
            KENNYKERR_DEFINE_STRUCT(SampleDescription, DXGI_SAMPLE_DESC)

            explicit SampleDescription(unsigned const count   = 1,
                                       unsigned const quality = 0) :
                Count(count),
                Quality(quality)
            {}

            unsigned Count;
            unsigned Quality;
        };

        struct SwapChainDescription1
        {
            KENNYKERR_DEFINE_STRUCT(SwapChainDescription1, DXGI_SWAP_CHAIN_DESC1)

            explicit SwapChainDescription1(Format const format         = Format::B8G8R8A8_UNORM,
                                           Usage const bufferUsage     = Usage::RenderTargetOutput,
                                           SwapEffect const swapEffect = SwapEffect::FlipSequential,
                                           unsigned const bufferCount  = 2) :
                Width(0),
                Height(0),
                Format(format),
                Stereo(FALSE),
                BufferUsage(bufferUsage),
                BufferCount(bufferCount),
                Scaling(Scaling::Stretch),
                SwapEffect(swapEffect),
                AlphaMode(KennyKerr::AlphaMode::Unknown),
                Flags(Dxgi::SwapChainFlag::None)
            {}

            unsigned Width;
            unsigned Height;
            Format Format;
            BOOL Stereo;
            SampleDescription Sample;
            Usage BufferUsage;
            unsigned BufferCount;
            Scaling Scaling;
            SwapEffect SwapEffect;
            AlphaMode AlphaMode;
            SwapChainFlag Flags;
        };

    } // Dxgi

    namespace Direct3D
    {
        struct TextureDescription2D
        {
            KENNYKERR_DEFINE_STRUCT(TextureDescription2D, D3D11_TEXTURE2D_DESC)

            explicit TextureDescription2D(Dxgi::Format const format          = Dxgi::Format::B8G8R8A8_UNORM,
                                          unsigned const width               = 0,
                                          unsigned const height              = 0,
                                          unsigned const arraySize           = 1,
                                          unsigned const mipLevels           = 0,
                                          BindFlag const bindFlags           = BindFlag::ShaderResource,
                                          Usage const usage                  = Usage::Default,
                                          CpuAccessFlag const cpuAccessFlags = CpuAccessFlag::None,
                                          ResourceMiscFlag const miscFlags   = ResourceMiscFlag::None) :
                Width(width),
                Height(height),
                MipLevels(mipLevels),
                ArraySize(arraySize),
                Format(format),
                Usage(usage),
                BindFlags(bindFlags),
                CpuAccessFlags(cpuAccessFlags),
                MiscFlags(miscFlags)
            {}

            unsigned Width;
            unsigned Height;
            unsigned MipLevels;
            unsigned ArraySize;
            Dxgi::Format Format;
            Dxgi::SampleDescription Sample;
            Usage Usage;
            BindFlag BindFlags;
            CpuAccessFlag CpuAccessFlags;
            ResourceMiscFlag MiscFlags;
        };

    } // Direct3D

    namespace Direct2D
    {
        struct DrawingStateDescription
        {
            KENNYKERR_DEFINE_STRUCT(DrawingStateDescription, D2D1_DRAWING_STATE_DESCRIPTION)

            explicit DrawingStateDescription(AntialiasMode const antialiasMode         = AntialiasMode::PerPrimitive,
                                             TextAntialiasMode const textAntialiasMode = TextAntialiasMode::Default,
                                             UINT64 const tag1                         = 0,
                                             UINT64 const tag2                         = 0,
                                             D2D1_MATRIX_3X2_F const & transform       = D2D1::IdentityMatrix()) :
                AntialiasMode(antialiasMode),
                TextAntialiasMode(textAntialiasMode),
                Tag1(tag1),
                Tag2(tag2),
                Transform(transform)
            {}

            AntialiasMode AntialiasMode;
            TextAntialiasMode TextAntialiasMode;
            UINT64 Tag1;
            UINT64 Tag2;
            D2D1_MATRIX_3X2_F Transform;
        };

        struct DrawingStateDescription1
        {
            KENNYKERR_DEFINE_STRUCT(DrawingStateDescription1, D2D1_DRAWING_STATE_DESCRIPTION1)

            explicit DrawingStateDescription1(AntialiasMode const antialiasMode         = AntialiasMode::PerPrimitive,
                                              TextAntialiasMode const textAntialiasMode = TextAntialiasMode::Default,
                                              UINT64 const tag1                         = 0,
                                              UINT64 const tag2                         = 0,
                                              D2D1_MATRIX_3X2_F const & transform       = D2D1::IdentityMatrix(),
                                              PrimitiveBlend const primitiveBlend       = PrimitiveBlend::SourceOver,
                                              UnitMode const unitMode                   = UnitMode::Dips) :
                AntialiasMode(antialiasMode),
                TextAntialiasMode(textAntialiasMode),
                Tag1(tag1),
                Tag2(tag2),
                Transform(transform),
                PrimitiveBlend(primitiveBlend),
                UnitMode(unitMode)
            {}

            AntialiasMode AntialiasMode;
            TextAntialiasMode TextAntialiasMode;
            UINT64 Tag1;
            UINT64 Tag2;
            D2D1_MATRIX_3X2_F Transform;
            PrimitiveBlend PrimitiveBlend;
            UnitMode UnitMode;
        };

        struct ArcSegment
        {
            KENNYKERR_DEFINE_STRUCT(ArcSegment, D2D1_ARC_SEGMENT)

            explicit ArcSegment(Point2F const & point               = Point2F(),
                                SizeF const & size                  = SizeF(),
                                float const rotationAngle           = 0.0f,
                                SweepDirection const sweepDirection = SweepDirection::Clockwise,
                                ArcSize const arcSize               = ArcSize::Small) :
                Point(point),
                Size(size),
                RotationAngle(rotationAngle),
                SweepDirection(sweepDirection),
                ArcSize(arcSize)
            {}

            Point2F Point;
            SizeF Size;
            float RotationAngle;
            SweepDirection SweepDirection;
            ArcSize ArcSize;
        };

        struct BezierSegment
        {
            KENNYKERR_DEFINE_STRUCT(BezierSegment, D2D1_BEZIER_SEGMENT)

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
            KENNYKERR_DEFINE_STRUCT(QuadraticBezierSegment, D2D1_QUADRATIC_BEZIER_SEGMENT)

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
            KENNYKERR_DEFINE_STRUCT(Triangle, D2D1_TRIANGLE)

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
            KENNYKERR_DEFINE_STRUCT(RoundedRect, D2D1_ROUNDED_RECT)

            explicit RoundedRect(RectF const & rect  = RectF(),
                                 float const radiusX = 0.0f,
                                 float const radiusY = 0.0f) :
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
            KENNYKERR_DEFINE_STRUCT(Ellipse, D2D1_ELLIPSE)

            explicit Ellipse(Point2F const & center = Point2F(),
                             float const radiusX    = 0.0f,
                             float const radiusY    = 0.0f) :
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
            KENNYKERR_DEFINE_STRUCT(GradientStop, D2D1_GRADIENT_STOP)

            explicit GradientStop(float const position = 0.0f,
                                  Color const & color  = KennyKerr::Color()) :
                Position(position),
                Color(color)
            {}

            float Position;
            Color Color;
        };

        struct PixelFormat
        {
            KENNYKERR_DEFINE_STRUCT(PixelFormat, D2D1_PIXEL_FORMAT)

            explicit PixelFormat(Dxgi::Format const format = Dxgi::Format::Unknown,
                                 AlphaMode const mode      = AlphaMode::Unknown) :
                Format(format),
                AlphaMode(mode)
            {}

            Dxgi::Format Format;
            AlphaMode AlphaMode;
        };

        struct PrintControlProperties
        {
            KENNYKERR_DEFINE_STRUCT(PrintControlProperties, D2D1_PRINT_CONTROL_PROPERTIES)

            explicit PrintControlProperties(PrintFontSubsetMode const fontSubset = PrintFontSubsetMode::Default,
                                            float const rasterDpi                = 150.0f,
                                            ColorSpace const colorSpace          = ColorSpace::sRGB) :
                FontSubset(fontSubset),
                RasterDpi(rasterDpi),
                ColorSpace(colorSpace)
            {}

            PrintFontSubsetMode FontSubset;
            float RasterDpi;
            ColorSpace ColorSpace;
        };

        struct CreationProperties
        {
            KENNYKERR_DEFINE_STRUCT(CreationProperties, D2D1_CREATION_PROPERTIES)

            explicit CreationProperties(ThreadingMode threadingMode = ThreadingMode::SingleThreaded,
                                        DebugLevel debugLevel = DebugLevel::None,
                                        DeviceContextOptions options = DeviceContextOptions::None) :
                ThreadingMode(threadingMode),
                DebugLevel(debugLevel),
                Options(options)
            {}

            ThreadingMode ThreadingMode;
            DebugLevel DebugLevel;
            DeviceContextOptions Options;
        };

        struct BrushProperties
        {
            KENNYKERR_DEFINE_STRUCT(BrushProperties, D2D1_BRUSH_PROPERTIES)

            explicit BrushProperties(float const opacity                 = 1.0,
                                     D2D1_MATRIX_3X2_F const & transform = D2D1::IdentityMatrix()) :
                Opacity(opacity),
                Transform(transform)
            {}

            float Opacity;
            D2D1_MATRIX_3X2_F Transform;
        };

        struct ImageBrushProperties
        {
            KENNYKERR_DEFINE_STRUCT(ImageBrushProperties, D2D1_IMAGE_BRUSH_PROPERTIES)

            explicit ImageBrushProperties(RectF const & sourceRectangle             = RectF(),
                                          ExtendMode const extendModeX              = ExtendMode::Clamp,
                                          ExtendMode const extendModeY              = ExtendMode::Clamp,
                                          InterpolationMode const interpolationMode = InterpolationMode::Linear) :
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
            KENNYKERR_DEFINE_STRUCT(BitmapProperties, D2D1_BITMAP_PROPERTIES)

            explicit BitmapProperties(PixelFormat const & format = Direct2D::PixelFormat(),
                                      float const dpiX           = 96.0f,
                                      float const dpiY           = 96.0f) :
                PixelFormat(format),
                DpiX(dpiX),
                DpiY(dpiY)
            {}

            PixelFormat PixelFormat;
            float DpiX;
            float DpiY;
        };

        struct BitmapProperties1
        {
            KENNYKERR_DEFINE_STRUCT(BitmapProperties1, D2D1_BITMAP_PROPERTIES1)

            explicit BitmapProperties1(BitmapOptions const options   = BitmapOptions::None,
                                       PixelFormat const & format    = Direct2D::PixelFormat(),
                                       float const dpiX              = 96.0f,
                                       float const dpiY              = 96.0f,
                                       ID2D1ColorContext * context   = nullptr) :
                PixelFormat(format),
                DpiX(dpiX),
                DpiY(dpiY),
                BitmapOptions(options),
                ColorContext(context)
            {}

            PixelFormat PixelFormat;
            float DpiX;
            float DpiY;
            BitmapOptions BitmapOptions;
            ID2D1ColorContext * ColorContext;
        };

        struct BitmapBrushProperties
        {
            KENNYKERR_DEFINE_STRUCT(BitmapBrushProperties, D2D1_BITMAP_BRUSH_PROPERTIES)

            explicit BitmapBrushProperties(ExtendMode const extendModeX                    = ExtendMode::Clamp,
                                           ExtendMode const extendModeY                    = ExtendMode::Clamp,
                                           BitmapInterpolationMode const interpolationMode = BitmapInterpolationMode::Linear) :
                ExtendModeX(extendModeX),
                ExtendModeY(extendModeY),
                InterpolationMode(interpolationMode)
            {}

            ExtendMode ExtendModeX;
            ExtendMode ExtendModeY;
            BitmapInterpolationMode InterpolationMode;
        };

        struct BitmapBrushProperties1
        {
            KENNYKERR_DEFINE_STRUCT(BitmapBrushProperties1, D2D1_BITMAP_BRUSH_PROPERTIES1)

            explicit BitmapBrushProperties1(ExtendMode const extendModeX              = ExtendMode::Clamp,
                                            ExtendMode const extendModeY              = ExtendMode::Clamp,
                                            InterpolationMode const interpolationMode = InterpolationMode::Linear) :
                ExtendModeX(extendModeX),
                ExtendModeY(extendModeY),
                InterpolationMode(interpolationMode)
            {}

            ExtendMode ExtendModeX;
            ExtendMode ExtendModeY;
            InterpolationMode InterpolationMode;
        };

        struct LinearGradientBrushProperties
        {
            KENNYKERR_DEFINE_STRUCT(LinearGradientBrushProperties, D2D1_LINEAR_GRADIENT_BRUSH_PROPERTIES)

            explicit LinearGradientBrushProperties(Point2F const & startPoint = Point2F(),
                                                   Point2F const & endPoint   = Point2F()) :
                StartPoint(startPoint),
                EndPoint(endPoint)
            {}

            Point2F StartPoint;
            Point2F EndPoint;
        };

        struct RadialGradientBrushProperties
        {
            KENNYKERR_DEFINE_STRUCT(RadialGradientBrushProperties, D2D1_RADIAL_GRADIENT_BRUSH_PROPERTIES)

            explicit RadialGradientBrushProperties(Point2F const & center = Point2F(),
                                                   Point2F const & offset = Point2F(),
                                                   float const radiusX    = 0.0f,
                                                   float const radiusY    = 0.0f) :
                Center(center),
                Offset(offset),
                RadiusX(radiusX),
                RadiusY(radiusY)
            {}

            Point2F Center;
            Point2F Offset;
            float RadiusX;
            float RadiusY;
        };

        struct StrokeStyleProperties
        {
            KENNYKERR_DEFINE_STRUCT(StrokeStyleProperties, D2D1_STROKE_STYLE_PROPERTIES)

            explicit StrokeStyleProperties(CapStyle const startCap   = CapStyle::Flat,
                                           CapStyle const endCap     = CapStyle::Flat,
                                           CapStyle const dashCap    = CapStyle::Flat,
                                           LineJoin const lineJoin   = LineJoin::Miter,
                                           float const miterLimit    = 10.0f,
                                           DashStyle const dashStyle = DashStyle::Solid,
                                           float const dashOffset    = 0.0f) :
                StartCap(startCap),
                EndCap(endCap),
                DashCap(dashCap),
                LineJoin(lineJoin),
                MiterLimit(miterLimit),
                DashStyle(dashStyle),
                DashOffset(dashOffset)
            {}

            CapStyle StartCap;
            CapStyle EndCap;
            CapStyle DashCap;
            LineJoin LineJoin;
            float MiterLimit;
            DashStyle DashStyle;
            float DashOffset;
        };

        struct StrokeStyleProperties1
        {
            KENNYKERR_DEFINE_STRUCT(StrokeStyleProperties1, D2D1_STROKE_STYLE_PROPERTIES1)

            explicit StrokeStyleProperties1(CapStyle const startCap                = CapStyle::Flat,
                                           CapStyle const endCap                   = CapStyle::Flat,
                                           CapStyle const dashCap                  = CapStyle::Flat,
                                           LineJoin const lineJoin                 = LineJoin::Miter,
                                           float const miterLimit                  = 10.0f,
                                           DashStyle const dashStyle               = DashStyle::Solid,
                                           float const dashOffset                  = 0.0f,
                                           StrokeTransformType const transformType = StrokeTransformType::Normal) :
                StartCap(startCap),
                EndCap(endCap),
                DashCap(dashCap),
                LineJoin(lineJoin),
                MiterLimit(miterLimit),
                DashStyle(dashStyle),
                DashOffset(dashOffset),
                TransformType(transformType)
            {}

            CapStyle StartCap;
            CapStyle EndCap;
            CapStyle DashCap;
            LineJoin LineJoin;
            float MiterLimit;
            DashStyle DashStyle;
            float DashOffset;
            StrokeTransformType TransformType;
        };

        struct LayerParameters
        {
            KENNYKERR_DEFINE_STRUCT(LayerParameters, D2D1_LAYER_PARAMETERS)

            explicit LayerParameters(RectF const & contentBounds             = RectF::Infinite(),
                                     ID2D1Geometry * geometricMask           = nullptr,
                                     AntialiasMode const maskAntialiasMode   = AntialiasMode::PerPrimitive,
                                     D2D1_MATRIX_3X2_F const & maskTransform = D2D1::IdentityMatrix(),
                                     float const opacity                     = 0.0f,
                                     ID2D1Brush * opacityBrush               = nullptr,
                                     LayerOptions const layerOptions         = LayerOptions::None) :
                ContentBounds(contentBounds),
                GeometricMask(geometricMask),
                MaskAntialiasMode(maskAntialiasMode),
                MaskTransform(maskTransform),
                Opacity(opacity),
                OpacityBrush(opacityBrush),
                LayerOptions(layerOptions)
            {}

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
            KENNYKERR_DEFINE_STRUCT(RenderTargetProperties, D2D1_RENDER_TARGET_PROPERTIES)

            explicit RenderTargetProperties(RenderTargetType const type     = RenderTargetType::Default,
                                            PixelFormat const & pixelFormat = Direct2D::PixelFormat(),
                                            float const dpiX                = 0.0f,
                                            float const dpiY                = 0.0f,
                                            RenderTargetUsage const usage   = RenderTargetUsage::None,
                                            FeatureLevel const minLevel     = FeatureLevel::Default) :
                Type(type),
                PixelFormat(pixelFormat),
                DpiX(dpiX),
                DpiY(dpiY),
                Usage(usage),
                MinLevel(minLevel)
            {}

            RenderTargetType Type;
            PixelFormat PixelFormat;
            float DpiX;
            float DpiY;
            RenderTargetUsage Usage;
            FeatureLevel MinLevel;
        };

        struct HwndRenderTargetProperties
        {
            KENNYKERR_DEFINE_STRUCT(HwndRenderTargetProperties, D2D1_HWND_RENDER_TARGET_PROPERTIES)

            explicit HwndRenderTargetProperties(HWND hwnd                           = nullptr,
                                                SizeU const & pixelSize             = SizeU(),
                                                PresentOptions const presentOptions = PresentOptions::None) :
                Hwnd(hwnd),
                PixelSize(pixelSize),
                PresentOptions(presentOptions)
            {}

            HWND Hwnd;
            SizeU PixelSize;
            PresentOptions PresentOptions;
        };

        struct MappedRect
        {
            KENNYKERR_DEFINE_STRUCT(MappedRect, D2D1_MAPPED_RECT)

            explicit MappedRect(unsigned const pitch = 0,
                                BYTE * bits = nullptr) :
                Pitch(pitch),
                Bits(bits)
            {}

            unsigned Pitch;
            BYTE * Bits;
        };

        struct RenderingControls
        {
            KENNYKERR_DEFINE_STRUCT(RenderingControls, D2D1_RENDERING_CONTROLS)

            explicit RenderingControls(BufferPrecision bufferPrecision = BufferPrecision::Unknown,
                                       SizeU const & tileSize = SizeU()) :
                BufferPrecision(bufferPrecision),
                TileSize(tileSize)
            {}

            BufferPrecision BufferPrecision;
            SizeU TileSize;
        };

        struct EffectInputDescription
        {
            KENNYKERR_DEFINE_STRUCT(EffectInputDescription, D2D1_EFFECT_INPUT_DESCRIPTION)

            explicit EffectInputDescription(ID2D1Effect * effect         = nullptr,
                                            unsigned const inputIndex    = 0,
                                            RectF const & inputRectangle = RectF()) :
                Effect(effect),
                InputIndex(inputIndex),
                InputRectangle(inputRectangle)
            {}

            ID2D1Effect * Effect;
            unsigned InputIndex;
            RectF InputRectangle;
        };

        struct PointDescription
        {
            KENNYKERR_DEFINE_STRUCT(PointDescription, D2D1_POINT_DESCRIPTION)

            explicit PointDescription(Point2F const & point             = Point2F(),
                                      Point2F const & unitTangentVector = Point2F(),
                                      unsigned const endSegment         = 0,
                                      unsigned const endFigure          = 0,
                                      float const lengthToEndSegment    = 0.0f) :
                Point(point),
                UnitTangentVector(unitTangentVector),
                EndSegment(endSegment),
                EndFigure(endFigure),
                LengthToEndSegment(lengthToEndSegment)
            {}

            Point2F Point;
            Point2F UnitTangentVector;
            unsigned EndSegment;
            unsigned EndFigure;
            float LengthToEndSegment;
        };

    } // Direct2D

    #pragma endregion Structures

    #pragma region Forward declarations

    struct ComInitialize;
    struct Stream;
    struct PropertyBag2;

    namespace Dxgi
    {
        struct Surface;
        struct SwapChain;
        struct SwapChain1;
        struct Resource;
        struct Factory2;
        struct Adapter;
        struct Device1;
    }

    namespace Direct3D
    {
        struct MultiThread;
        struct Texture2D;
        struct DeviceContext;
        struct Device;
    }

    namespace Wic
    {
        struct Palette;
        struct BitmapSource;
        struct BitmapLock;
        struct Bitmap;
        struct BitmapFrameDecode;
        struct ColorContext;
        struct FormatConverter;
        struct BitmapFrameEncode;
        struct BitmapEncoder;
        struct BitmapDecoder;
        struct Stream;
        struct Factory;
    }

    namespace Wam
    {
        struct Manager;
        struct TransitionLibrary;
        struct Transition;
        struct Variable;
    }

    namespace DirectWrite
    {
        struct RenderingParams;
        struct TextFormat;
        struct TextLayout;
    }

    namespace Direct2D
    {
        struct SimplifiedGeometrySink;
        struct TessellationSink;
        struct Resource;
        struct Image;
        struct Bitmap;
        struct ColorContext;
        struct Bitmap1;
        struct GradientStopCollection;
        struct GradientStopCollection1;
        struct Brush;
        struct BitmapBrush;
        struct BitmapBrush1;
        struct SolidColorBrush;
        struct LinearGradientBrush;
        struct RadialGradientBrush;
        struct StrokeStyle;
        struct StrokeStyle1;
        struct Geometry;
        struct RectangleGeometry;
        struct RoundedRectangleGeometry;
        struct EllipseGeometry;
        struct GeometryGroup;
        struct TransformedGeometry;
        struct GeometrySink;
        struct PathGeometry;
        struct PathGeometry1;
        struct Properties;
        struct Effect;
        struct Mesh;
        struct Layer;
        struct DrawingStateBlock;
        struct DrawingStateBlock1;
        struct RenderTarget;
        struct BitmapRenderTarget;
        struct HwndRenderTarget;
        struct GdiInteropRenderTarget;
        struct DcRenderTarget;
        struct GdiMetafileSink;
        struct GdiMetafile;
        struct CommandSink;
        struct CommandList;
        struct PrintControl;
        struct ImageBrush;
        struct DeviceContext;
        struct Device;
        struct MultiThread;
        struct Factory;
        struct Factory1;
    }

    #pragma endregion Forward declarations

    #pragma region Classes

    struct ComInitialize
    { 
        explicit ComInitialize(Apartment apartment = Apartment::MultiThreaded);
        ~ComInitialize();
    };

    struct Stream : Details::Object
    {
        KENNYKERR_DEFINE_CLASS(Stream, Details::Object, IStream)
    };

    struct PropertyBag2 : Details::Object
    {
        KENNYKERR_DEFINE_CLASS(PropertyBag2, Details::Object, IPropertyBag2)
    };

    namespace Dxgi
    {
        struct __declspec(uuid("cafcb56c-6ac3-4889-bf47-9e23bbd260ec")) Surface : Details::Object
        {
            KENNYKERR_DEFINE_CLASS(Surface, Details::Object, IDXGISurface)
        };

        struct SwapChain : Details::Object
        {
            KENNYKERR_DEFINE_CLASS(SwapChain, Details::Object, IDXGISwapChain)

            auto Present(unsigned const sync = 1,
                         Present const flags = Present::None) const -> HRESULT;

            auto GetBuffer(unsigned const index = 0) const -> Surface;

            auto ResizeBuffers(unsigned const width = 0,
                               unsigned const height = 0) const -> HRESULT;
        };

        struct SwapChain1 : SwapChain
        {
            KENNYKERR_DEFINE_CLASS(SwapChain1, SwapChain, IDXGISwapChain1)
        };

        struct Resource : Details::Object
        {
            KENNYKERR_DEFINE_CLASS(Resource, Details::Object, IDXGIResource)

            auto Resource::GetSharedHandle() const -> HANDLE;
        };

        struct __declspec(uuid("50c83a1c-e072-4c48-87b0-3630fa36a6d0")) Factory2 : Details::Object
        {
            KENNYKERR_DEFINE_CLASS(Factory2, Details::Object, IDXGIFactory2)

            auto Factory2::CreateSwapChainForHwnd(Details::Object const & device, // Direct3D or Dxgi Device
                                                  HWND window,
                                                  SwapChainDescription1 const & description) const -> SwapChain;

            auto Factory2::CreateSwapChainForCoreWindow(Details::Object const & device,  // Direct3D or Dxgi Device
                                                        IUnknown * window,
                                                        SwapChainDescription1 const & description) const -> SwapChain;

            #ifdef __cplusplus_winrt
            auto Factory2::CreateSwapChainForCoreWindow(Device const & device,  // Direct3D or Dxgi Device
                                                        Windows::UI::Core::CoreWindow ^ window,
                                                        SwapChainDescription1 const & description) const -> SwapChain;
            #endif

            auto Factory2::RegisterOcclusionStatusWindow(HWND window,
                                                         unsigned const message = WM_USER) const -> DWORD;

            void Factory2::UnregisterOcclusionStatus(DWORD cookie) const;
        };

        struct Adapter : Details::Object
        {
            KENNYKERR_DEFINE_CLASS(Adapter, Details::Object, IDXGIAdapter)

            auto GetParent() const -> Factory2;
        };

        struct Device : Details::Object
        {
            KENNYKERR_DEFINE_CLASS(Device, Details::Object, IDXGIDevice)

            auto GetAdapter() const -> Adapter;
        };

        struct Device1 : Device
        {
            KENNYKERR_DEFINE_CLASS(Device1, Device, IDXGIDevice1)
        };
    }

    namespace Direct3D
    {
        struct MultiThread : Details::Object
        {
            KENNYKERR_DEFINE_CLASS(MultiThread, Details::Object, ID3D10Multithread)

            void MultiThreadEnter() const;
            void Leave() const;
            auto SetMultithreadProtected(bool protect = true) const -> bool;
            auto GetMultithreadProtected() const -> bool;
        };

        struct Texture2D : Details::Object
        {
            KENNYKERR_DEFINE_CLASS(Texture2D, Details::Object, ID3D11Texture2D)

            auto AsDxgiResource() const -> Dxgi::Resource;
        };

        struct DeviceContext : Details::Object
        {
            KENNYKERR_DEFINE_CLASS(DeviceContext, Details::Object, ID3D11DeviceContext)

            void Flush() const;
        };

        struct Device : Details::Object
        {
            KENNYKERR_DEFINE_CLASS(Device, Details::Object, ID3D11Device)

            auto AsDxgi() const -> Dxgi::Device1;
            auto AsMultiThread() const -> MultiThread;
            auto GetDxgiFactory() const -> Dxgi::Factory2;

            auto CreateTexture2D(TextureDescription2D const & description) const -> Texture2D;
            auto GetImmediateContext() const -> DeviceContext;
            auto OpenSharedResource(HANDLE resource) const -> Dxgi::Surface;
            auto OpenSharedResource(Dxgi::Resource const & resource) const -> Dxgi::Surface;
            auto OpenSharedResource(Texture2D const & resource) const -> Dxgi::Surface;
        };

        struct Device1 : Device
        {
            KENNYKERR_DEFINE_CLASS(Device1, Device, ID3D11Device1)
        };

    }

    namespace Wic
    {
        struct Palette : Details::Object
        {
            KENNYKERR_DEFINE_CLASS(Palette, Details::Object, IWICPalette)
        };

        struct BitmapSource : Details::Object
        {
            KENNYKERR_DEFINE_CLASS(BitmapSource, Details::Object, IWICBitmapSource)

            auto GetSize() const -> SizeU;
            void GetPixelFormat(GUID & format) const;
        };

        struct __declspec(uuid("00000123-a8f2-4877-ba0a-fd2b6645fb94")) BitmapLock : Details::Object
        {
            KENNYKERR_DEFINE_CLASS(BitmapLock, Details::Object, IWICBitmapLock)
        };

        struct Bitmap : BitmapSource
        {
            KENNYKERR_DEFINE_CLASS(Bitmap, BitmapSource, IWICBitmap)
        };

        struct BitmapFrameDecode : BitmapSource
        {
            KENNYKERR_DEFINE_CLASS(BitmapFrameDecode, BitmapSource, IWICBitmapFrameDecode)
        };

        struct ColorContext : Details::Object
        {
            KENNYKERR_DEFINE_CLASS(ColorContext, Details::Object, IWICColorContext)
        };

        struct FormatConverter : BitmapSource
        {
            KENNYKERR_DEFINE_CLASS(FormatConverter, BitmapSource, IWICFormatConverter);

            void Initialize(BitmapSource const & source,
                            GUID const & format                = GUID_WICPixelFormat32bppPBGRA,
                            BitmapDitherType dither            = BitmapDitherType::None,
                            double alphaThresholdPercent       = 0.0,
                            BitmapPaletteType paletteTranslate = BitmapPaletteType::MedianCut) const;
        };

        struct BitmapFrameEncode : Details::Object
        {
            KENNYKERR_DEFINE_CLASS(BitmapFrameEncode, Details::Object, IWICBitmapFrameEncode)

            void Initialize(PropertyBag2 const & properties) const;
            void SetSize(SizeU const & size) const;
            void SetPixelFormat(GUID & format) const;
            void WriteSource(BitmapSource const & source) const;

            void WriteSource(BitmapSource const & source,
                             RectU const & rect) const;

            void Commit() const;
        };

        struct BitmapEncoder : Details::Object
        {
            KENNYKERR_DEFINE_CLASS(BitmapEncoder, Details::Object, IWICBitmapEncoder)

            void Initialize(Stream const & stream,
                            BitmapEncoderCacheOption cache = BitmapEncoderCacheOption::None) const;

            void CreateNewFrame(BitmapFrameEncode & frame,
                                PropertyBag2 & properties) const;

            auto CreateNewFrame() const -> BitmapFrameEncode;

            void Commit() const;
        };

        struct BitmapDecoder : Details::Object
        {
            KENNYKERR_DEFINE_CLASS(BitmapDecoder, Details::Object, IWICBitmapDecoder)

            auto GetFrameCount() const -> unsigned;
            auto GetFrame(unsigned index = 0) const -> BitmapFrameDecode;
        };

        struct Stream : KennyKerr::Stream
        {
            KENNYKERR_DEFINE_CLASS(Stream, KennyKerr::Stream, IWICStream)

            void InitializeFromMemory(BYTE * buffer,
                                      unsigned size) const;

            auto InitializeFromFilename(PCWSTR filename,
                                        DWORD desiredAccess = GENERIC_READ | GENERIC_WRITE) -> HRESULT;
        };

        struct Factory : Details::Object
        {
            KENNYKERR_DEFINE_CLASS(Factory, Details::Object, IWICImagingFactory)

            auto CreateBitmap(SizeU const & size,
                              GUID const & format           = GUID_WICPixelFormat32bppPBGRA,
                              BitmapCreateCacheOption cache = BitmapCreateCacheOption::OnLoad) const -> Bitmap;

            auto CreateEncoder(GUID const & format) const -> BitmapEncoder;

            auto CreateDecoderFromStream(KennyKerr::Stream const & stream,
                                         DecodeCacheOption options = DecodeCacheOption::OnDemand) const -> BitmapDecoder;

            auto CreateFormatConverter() const -> FormatConverter;
            auto CreateStream() const -> Stream;
        };

        struct ImageEncoder : Details::Object
        {
            KENNYKERR_DEFINE_CLASS(ImageEncoder, Details::Object, IWICImageEncoder)
        };

        struct Factory2 : Factory
        {
            KENNYKERR_DEFINE_CLASS(Factory2, Factory, IWICImagingFactory2)

            auto CreateImageEncoder(Direct2D::Device const & device) const -> ImageEncoder;
        };
    }

    namespace Wam
    {
        struct Manager : Details::Object
        {
            KENNYKERR_DEFINE_CLASS(Manager, Details::Object, IUIAnimationManager)

            auto CreateAnimationVariable(double initialValue = 0.0) const -> Variable;

            void ScheduleTransition(Variable const & variable,
                                    Transition const & transition,
                                    double time) const;
        };

        struct TransitionLibrary : Details::Object
        {
            KENNYKERR_DEFINE_CLASS(TransitionLibrary, Details::Object, IUIAnimationTransitionLibrary)

            auto CreateAccelerateDecelerateTransition(double duration,
                                                      double finalValue,
                                                      double accelerationRatio,
                                                      double decelerationRatio) const -> Transition;
        };

        struct Transition : Details::Object
        {
            KENNYKERR_DEFINE_CLASS(Transition, Details::Object, IUIAnimationTransition)
        };

        struct Variable : Details::Object
        {
            KENNYKERR_DEFINE_CLASS(Variable, Details::Object, IUIAnimationVariable)
        };
    }

    namespace DirectWrite
    {
        struct RenderingParams : Details::Object
        {
            KENNYKERR_DEFINE_CLASS(RenderingParams, Details::Object, IDWriteRenderingParams)

            auto GetGamma() const -> float;
            auto GetEnhancedContrast() const -> float;
            auto GetClearTypeLevel() const -> float;
            auto GetPixelGeometry() const -> PixelGeometry;
            auto GetRenderingMode() const -> RenderingMode;
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
        struct SimplifiedGeometrySink : Details::Object
        {
            KENNYKERR_DEFINE_CLASS(SimplifiedGeometrySink, Details::Object, ID2D1SimplifiedGeometrySink)

            void SetFillMode(FillMode mode) const;
            void SetSegmentFlags(PathSegment flags) const;

            void BeginFigure(Point2F const & startPoint,
                             FigureBegin figureBegin) const;

            void AddLines(Point2F const * points,
                          unsigned count) const;

            template <size_t Count>
            void AddLines(Point2F const (&points)[Count]) const
            {
                AddLines(points,
                         Count);
            }

            void AddBeziers(BezierSegment const * beziers,
                            unsigned count) const;

            template <size_t Count>
            void AddBeziers(BezierSegment const (&beziers)[Count]) const
            {
                AddBeziers(beziers,
                           Count);
            }

            void EndFigure(FigureEnd figureEnd) const;
        };

        struct TessellationSink : Details::Object
        {
            KENNYKERR_DEFINE_CLASS(TessellationSink, Details::Object, ID2D1TessellationSink)

            void AddTriangles(Triangle const * triangles,
                              unsigned count) const;

            template <size_t Count>
            void AddTriangles(Triangle const (&triangles)[Count]) const
            {
                AddTriangles(triangles,
                             Count);
            }

            void Close();
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

            auto GetSize() const -> SizeF;
            auto GetPixelSize() const -> SizeU;
            auto GetPixelFormat() const -> PixelFormat;
            void GetDpi(float & x, float & y) const;
            void CopyFromBitmap(Bitmap const & other) const;

            void CopyFromBitmap(Bitmap const & other,
                                Point2U const & destination) const;

            void CopyFromBitmap(Bitmap const & other,
                                RectU const & source) const;

            void CopyFromBitmap(Bitmap const & other,
                                Point2U const & destination,
                                RectU const & source) const;

            void CopyFromRenderTarget(RenderTarget const & other) const;

            void CopyFromRenderTarget(RenderTarget const & other,
                                      Point2U const & destination) const;

            void CopyFromRenderTarget(RenderTarget const & other,
                                      RectU const & source) const;

            void CopyFromRenderTarget(RenderTarget const & other,
                                      Point2U const & destination,
                                      RectU const & source) const;

            void CopyFromMemory(void const * data,
                                unsigned pitch) const;

            void CopyFromMemory(void const * data,
                                unsigned pitch,
                                RectU const & destination) const;
        };

        struct ColorContext : Resource
        {
            KENNYKERR_DEFINE_CLASS(ColorContext, Resource, ID2D1ColorContext)

            auto GetColorSpace() const -> ColorSpace;
            auto GetProfileSize() const -> unsigned;

            void GetProfile(BYTE * profile,
                            unsigned size) const;
        };

        struct Bitmap1 : Bitmap
        {
            KENNYKERR_DEFINE_CLASS(Bitmap1, Bitmap, ID2D1Bitmap1)

            auto GetColorContext() const -> ColorContext;
            auto GetOptions() const -> BitmapOptions;
            auto GetSurface() const -> Dxgi::Surface;

            void Map(MapOptions options,
                     MappedRect & mappedRect) const;

            void Unmap() const;
        };

        struct GradientStopCollection : Resource
        {
            KENNYKERR_DEFINE_CLASS(GradientStopCollection, Resource, ID2D1GradientStopCollection)

            auto GetGradientStopCount() const -> unsigned;

            void GetGradientStops(GradientStop * stops,
                                  unsigned count) const;

            template <size_t Count>
            void GetGradientStops(GradientStop (&stops)[Count]) const
            {
                GetGradientStops(stops, Count);
            }

            auto GetColorInterpolationGamma() const -> Gamma;
            auto GetExtendMode() const -> ExtendMode;
        };

        struct GradientStopCollection1 : GradientStopCollection
        {
            KENNYKERR_DEFINE_CLASS(GradientStopCollection1, GradientStopCollection, ID2D1GradientStopCollection1)

            void GetGradientStops1(GradientStop * stops,
                                  unsigned count) const;

            template <size_t Count>
            void GetGradientStops1(GradientStop (&stops)[Count]) const
            {
                GetGradientStops1(stops, Count);
            }

            auto GetPreInterpolationSpace() const -> ColorSpace;
            auto GetPostInterpolationSpace() const -> ColorSpace;
            auto GetBufferPrecision() const -> BufferPrecision;
            auto GetColorInterpolationMode() const -> ColorInterpolationMode;
        };

        struct Brush : Resource
        {
            KENNYKERR_DEFINE_CLASS(Brush, Resource, ID2D1Brush)

            void SetOpacity(float opacity) const;
            auto GetOpacity() const -> float;
            void GetTransform(D2D1_MATRIX_3X2_F & transform) const;
            void SetTransform(D2D1_MATRIX_3X2_F const & transform) const;
        };

        struct BitmapBrush : Brush
        {
            KENNYKERR_DEFINE_CLASS(BitmapBrush, Brush, ID2D1BitmapBrush)

            void SetExtendModeX(ExtendMode mode) const;
            void SetExtendModeY(ExtendMode mode) const;
            void SetInterpolationMode(BitmapInterpolationMode mode) const;
            void SetBitmap(Bitmap const & bitmap) const;
            auto GetExtendModeX() const -> ExtendMode;
            auto GetExtendModeY() const -> ExtendMode;
            auto GetInterpolationMode() const -> BitmapInterpolationMode;
            auto GetBitmap() const -> Bitmap;
        };

        struct BitmapBrush1 : BitmapBrush
        {
            KENNYKERR_DEFINE_CLASS(BitmapBrush1, BitmapBrush, ID2D1BitmapBrush1)

            void SetInterpolationMode1(InterpolationMode mode) const;
            auto GetInterpolationMode1() const -> InterpolationMode;
        };

        struct SolidColorBrush : Brush
        {
            KENNYKERR_DEFINE_CLASS(SolidColorBrush, Brush, ID2D1SolidColorBrush)

            void SetColor(Color const & color) const;
            auto GetColor() const -> Color;
        };

        struct LinearGradientBrush : Brush
        {
            KENNYKERR_DEFINE_CLASS(LinearGradientBrush, Brush, ID2D1LinearGradientBrush)

            void SetStartPoint(Point2F const & point) const;
            void SetEndPoint(Point2F const & point) const;
            auto GetStartPoint() const -> Point2F;
            auto GetEndPoint() const -> Point2F;
            auto GetGradientStopCollection() const -> GradientStopCollection;
        };

        struct RadialGradientBrush : Brush
        {
            KENNYKERR_DEFINE_CLASS(RadialGradientBrush, Brush, ID2D1RadialGradientBrush)

            void SetCenter(Point2F const & point) const;
            void SetGradientOriginOffset(Point2F const & point) const;
            void SetRadiusX(float radius) const;
            void SetRadiusY(float radius) const;
            auto GetCenter() const -> Point2F;
            auto GetGradientOriginOffset() const -> Point2F;
            auto GetRadiusX() const -> float;
            auto GetRadiusY() const -> float;
            auto GetGradientStopCollection() const -> GradientStopCollection;
        };

        struct StrokeStyle : Resource
        {
            KENNYKERR_DEFINE_CLASS(StrokeStyle, Resource, ID2D1StrokeStyle)

            auto GetStartCap() const -> CapStyle;
            auto GetEndCap() const -> CapStyle;
            auto GetDashCap() const -> CapStyle;
            auto GetMiterLimit() const -> float;
            auto GetLineJoin() const -> LineJoin;
            auto GetDashOffset() const -> float;
            auto GetDashStyle() const -> DashStyle;
            auto GetDashesCount() const -> unsigned;

            void GetDashes(float * dashes,
                           unsigned count) const;

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

            auto GetStrokeTransformType() const -> StrokeTransformType;
        };

    } // Direct2D

    #pragma endregion Classes

    #pragma region Functions

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
        inline auto CreateFactory() -> Factory2
        {
            Factory2 result;

            HR(CreateDXGIFactory1(__uuidof(result),
                                  reinterpret_cast<void **>(result.GetAddressOf())));

            return result;
        }
    }

    namespace Direct3D
    {
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
        inline auto CreateFactory() -> Factory2
        {
            Factory2 result;

            HR(CoCreateInstance(CLSID_WICImagingFactory,
                                result.GetAddressOf()));

            return result;
        }
    }

    namespace Wam
    {
        inline auto CreateManager() -> Manager
        {
            Manager result;

            HR(CoCreateInstance(__uuidof(UIAnimationManager),
                                result.GetAddressOf()));

            return result;
        }

        inline auto CreateTransitionLibrary() -> TransitionLibrary
        {
            TransitionLibrary result;

            HR(CoCreateInstance(__uuidof(UIAnimationTransitionLibrary),
                                result.GetAddressOf()));

            return result;
        }
    }

    namespace Direct2D
    {
    }

    #pragma endregion Functions

    #pragma region Implementation

    inline ComInitialize::ComInitialize(Apartment apartment)
    {
        HR(CoInitializeEx(nullptr,
                          static_cast<DWORD>(apartment)));
    }

    inline ComInitialize::~ComInitialize()
    {
        CoUninitialize();
    }

    namespace Dxgi
    {
        inline auto SwapChain::Present(unsigned const sync,
                                       Dxgi::Present const flags) const -> HRESULT
        {
            return (*this)->Present(sync, static_cast<unsigned>(flags));
        }

        inline auto SwapChain::GetBuffer(unsigned const index) const -> Surface
        {
            Surface result;

            HR((*this)->GetBuffer(index,
                                  __uuidof(result),
                                  reinterpret_cast<void **>(result.GetAddressOf())));

            return result;
        }

        inline auto SwapChain::ResizeBuffers(unsigned const width,
                                             unsigned const height) const -> HRESULT
        {
            return (*this)->ResizeBuffers(0, // all buffers
                                          width,
                                          height,
                                          DXGI_FORMAT_UNKNOWN, // preserve format
                                          0); // flags
        }

        inline auto Resource::GetSharedHandle() const -> HANDLE
        {
            HANDLE result;
            HR((*this)->GetSharedHandle(&result));
            return result;
        }

        inline auto Factory2::CreateSwapChainForHwnd(Details::Object const & device, // Direct3D or Dxgi Device
                                                     HWND window,
                                                     SwapChainDescription1 const & description) const -> SwapChain
        {
            SwapChain1 result;

            HR((*this)->CreateSwapChainForHwnd(device.Unknown(),
                                               window,
                                               description.Get(),
                                               nullptr, // windowed
                                               nullptr, // no restrictions
                                               result.GetAddressOf()));

            return result;
        }

        inline auto Factory2::CreateSwapChainForCoreWindow(Details::Object const & device,  // Direct3D or Dxgi Device
                                                           IUnknown * window,
                                                           SwapChainDescription1 const & description) const -> SwapChain
        {
            SwapChain1 result;

            HR((*this)->CreateSwapChainForCoreWindow(device.Unknown(),
                                                     window,
                                                     description.Get(),
                                                     nullptr, // no restrictions
                                                     result.GetAddressOf()));

            return result;
        }

        #ifdef __cplusplus_winrt
        inline auto Factory2::CreateSwapChainForCoreWindow(Device const & device,  // Direct3D or Dxgi Device
                                                           Windows::UI::Core::CoreWindow ^ window,
                                                           SwapChainDescription1 const & description) const -> SwapChain
        {
            return CreateSwapChainForCoreWindow(device,
                                                reinterpret_cast<IUnknown *>(window),
                                                description);
        }
        #endif

        inline auto Factory2::RegisterOcclusionStatusWindow(HWND window,
                                                            unsigned const message) const -> DWORD
        {
            DWORD cookie;

            HR((*this)->RegisterOcclusionStatusWindow(window,
                                                      message,
                                                      &cookie));

            return cookie;
        }

        inline void Factory2::UnregisterOcclusionStatus(DWORD cookie) const
        {
            (*this)->UnregisterOcclusionStatus(cookie);
        }

        inline auto Adapter::GetParent() const -> Factory2
        {
            Factory2 result;

            HR((*this)->GetParent(__uuidof(result),
                                  reinterpret_cast<void **>(result.GetAddressOf())));

            return result;
        }

        inline auto Device::GetAdapter() const -> Adapter
        {
            Adapter result;
            HR((*this)->GetAdapter(result.GetAddressOf()));
            return result;
        }
    }

    namespace Direct3D
    {
        inline void MultiThread::MultiThreadEnter() const
        {
            (*this)->Enter();
        }

        inline void MultiThread::Leave() const
        {
            (*this)->Leave();
        }

        inline auto MultiThread::SetMultithreadProtected(bool protect) const -> bool
        {
            return 0 != (*this)->SetMultithreadProtected(protect);
        }

        inline auto MultiThread::GetMultithreadProtected() const -> bool
        {
            return 0 != (*this)->GetMultithreadProtected();
        }

        inline auto Texture2D::AsDxgiResource() const -> Dxgi::Resource
        {
            Dxgi::Resource result;
            HR(m_ptr.CopyTo(result.GetAddressOf()));
            return result;
        }

        inline void DeviceContext::Flush() const
        {
            (*this)->Flush();
        }

        inline auto Device::AsDxgi() const -> Dxgi::Device1
        {
            Dxgi::Device1 result;
            HR(m_ptr.CopyTo(result.GetAddressOf()));
            return result;
        }

        inline auto Device::AsMultiThread() const -> MultiThread
        {
            MultiThread result;
            HR(m_ptr.CopyTo(result.GetAddressOf()));
            return result;
        }

        inline auto Device::GetDxgiFactory() const -> Dxgi::Factory2
        {
            return AsDxgi().GetAdapter().GetParent();
        }

        inline auto Device::CreateTexture2D(TextureDescription2D const & description) const -> Texture2D
        {
            Texture2D result;

            HR((*this)->CreateTexture2D(description.Get(),
                                        nullptr,
                                        result.GetAddressOf()));

            return result;
        }

        inline auto Device::GetImmediateContext() const -> DeviceContext
        {
            DeviceContext result;
            (*this)->GetImmediateContext(result.GetAddressOf());
            return result;
        };

        inline auto Device::OpenSharedResource(HANDLE resource) const -> Dxgi::Surface
        {
            Dxgi::Surface result;

            HR((*this)->OpenSharedResource(resource,
                                           __uuidof(result),
                                           reinterpret_cast<void **>(result.GetAddressOf())));

            return result;
        }

        inline auto Device::OpenSharedResource(Dxgi::Resource const & resource) const -> Dxgi::Surface
        {
            return OpenSharedResource(resource.GetSharedHandle());
        }

        inline auto Device::OpenSharedResource(Texture2D const & resource) const -> Dxgi::Surface
        {
            return OpenSharedResource(resource.AsDxgiResource().GetSharedHandle());
        }

    }

    namespace Wic
    {
        inline auto BitmapSource::GetSize() const -> SizeU
        {
            SizeU result;
            HR((*this)->GetSize(&result.Width, &result.Height));
            return result;
        }

        inline void BitmapSource::GetPixelFormat(GUID & format) const
        {
            HR((*this)->GetPixelFormat(&format));
        }

        inline void FormatConverter::Initialize(BitmapSource const & source,
                                                GUID const & format,
                                                BitmapDitherType dither,
                                                double alphaThresholdPercent,
                                                BitmapPaletteType paletteTranslate) const
        {
            HR((*this)->Initialize(source.Get(),
                                   format,
                                   static_cast<WICBitmapDitherType>(dither),
                                   nullptr,
                                   alphaThresholdPercent,
                                   static_cast<WICBitmapPaletteType>(paletteTranslate)));
        }

        inline void BitmapFrameEncode::Initialize(PropertyBag2 const & properties) const
        {
            HR((*this)->Initialize(properties.Get()));
        }

        inline void BitmapFrameEncode::SetSize(SizeU const & size) const
        {
            HR((*this)->SetSize(size.Width,
                                size.Height));
        }

        inline void BitmapFrameEncode::SetPixelFormat(GUID & format) const
        {
            HR((*this)->SetPixelFormat(&format));
        }

        inline void BitmapFrameEncode::WriteSource(BitmapSource const & source) const
        {
            HR((*this)->WriteSource(source.Get(),
                                    nullptr));
        }

        inline void BitmapFrameEncode::WriteSource(BitmapSource const & source,
                                                   RectU const & rect) const
        {
            WICRect wrect = { rect.Left, rect.Top, rect.Width(), rect.Height() };

            HR((*this)->WriteSource(source.Get(),
                                    &wrect));
        }

        inline void BitmapFrameEncode::Commit() const
        {
            HR((*this)->Commit());
        }

        inline void BitmapEncoder::Initialize(Stream const & stream,
                                              BitmapEncoderCacheOption cache) const
        {
            HR((*this)->Initialize(stream.Get(),
                                   static_cast<WICBitmapEncoderCacheOption>(cache)));
        }

        inline void BitmapEncoder::CreateNewFrame(BitmapFrameEncode & frame,
                                                  PropertyBag2 & properties) const
        {
            HR((*this)->CreateNewFrame(frame.GetAddressOf(),
                                       properties.GetAddressOf()));
        }

        inline auto BitmapEncoder::CreateNewFrame() const -> BitmapFrameEncode
        {
            BitmapFrameEncode frame;
            PropertyBag2 properties;

            CreateNewFrame(frame,
                           properties);

            frame.Initialize(properties);
            return frame;
        }

        inline void BitmapEncoder::Commit() const
        {
            HR((*this)->Commit());
        }

        inline auto BitmapDecoder::GetFrameCount() const -> unsigned
        {
            unsigned count;
            HR((*this)->GetFrameCount(&count));
            return count;
        }

        inline auto BitmapDecoder::GetFrame(unsigned index) const -> BitmapFrameDecode
        {
            BitmapFrameDecode result;

            HR((*this)->GetFrame(index,
                                 result.GetAddressOf()));

            return result;
        }

        inline void Stream::InitializeFromMemory(BYTE * buffer,
                                                 unsigned size) const
        {
            HR((*this)->InitializeFromMemory(buffer,
                                             size));
        }

        inline auto Stream::InitializeFromFilename(PCWSTR filename,
                                                   DWORD desiredAccess) -> HRESULT
        {
            return (*this)->InitializeFromFilename(filename,
                                                   desiredAccess);
        }

        inline auto Factory::CreateBitmap(SizeU const & size,
                                          GUID const & format,
                                          BitmapCreateCacheOption cache) const -> Bitmap
        {
            Bitmap result;

            HR((*this)->CreateBitmap(size.Width,
                                     size.Height,
                                     format,
                                     static_cast<WICBitmapCreateCacheOption>(cache),
                                     result.GetAddressOf()));

            return result;
        }

        inline auto Factory::CreateEncoder(GUID const & format) const -> BitmapEncoder
        {
            BitmapEncoder result;

            HR((*this)->CreateEncoder(format,
                                        nullptr,
                                        result.GetAddressOf()));

            return result;
        }

        inline auto Factory::CreateDecoderFromStream(KennyKerr::Stream const & stream,
                                                     DecodeCacheOption options) const -> BitmapDecoder
        {
            BitmapDecoder result;

            HR((*this)->CreateDecoderFromStream(stream.Get(),
                                                nullptr,
                                                static_cast<WICDecodeOptions>(options),
                                                result.GetAddressOf()));

            return result;
        }

        inline auto Factory::CreateFormatConverter() const -> FormatConverter
        {
            FormatConverter result;
            HR((*this)->CreateFormatConverter(result.GetAddressOf()));
            return result;
        }

        inline auto Factory::CreateStream() const -> Stream
        {
            Stream result;
            HR((*this)->CreateStream(result.GetAddressOf()));
            return result;
        }

        inline auto Factory2::CreateImageEncoder(Direct2D::Device const & device) const -> ImageEncoder
        {
            ImageEncoder result;

            HR((*this)->CreateImageEncoder(device.Get(),
                                           result.GetAddressOf()));

            return result;
        }
    }

    namespace Wam
    {
        inline auto Manager::CreateAnimationVariable(double initialValue) const -> Variable
        {
            Variable result;

            HR((*this)->CreateAnimationVariable(initialValue,
                                                result.GetAddressOf()));

            return result;
        };

        inline void Manager::ScheduleTransition(Variable const & variable,
                                                Transition const & transition,
                                                double time) const
        {
            HR((*this)->ScheduleTransition(variable.Get(),
                                           transition.Get(),
                                           time));
        }

        inline auto TransitionLibrary::CreateAccelerateDecelerateTransition(double duration,
                                                                            double finalValue,
                                                                            double accelerationRatio,
                                                                            double decelerationRatio) const -> Transition
        {
            Transition result;

            HR((*this)->CreateAccelerateDecelerateTransition(duration,
                                                             finalValue,
                                                             accelerationRatio,
                                                             decelerationRatio,
                                                             result.GetAddressOf()));

            return result;
        }
    }

    namespace DirectWrite
    {
        inline auto RenderingParams::GetGamma() const -> float
        {
            return (*this)->GetGamma();
        }

        inline auto RenderingParams::GetEnhancedContrast() const -> float
        {
            return (*this)->GetEnhancedContrast();
        }

        inline auto RenderingParams::GetClearTypeLevel() const -> float
        {
            return (*this)->GetClearTypeLevel();
        }

        inline auto RenderingParams::GetPixelGeometry() const -> PixelGeometry
        {
            return static_cast<PixelGeometry>((*this)->GetPixelGeometry());
        }

        inline auto RenderingParams::GetRenderingMode() const -> RenderingMode
        {
            return static_cast<RenderingMode>((*this)->GetRenderingMode());
        }
    }

    namespace Direct2D
    {
        inline void SimplifiedGeometrySink::SetFillMode(FillMode mode) const
        {
            (*this)->SetFillMode(static_cast<D2D1_FILL_MODE>(mode));
        }

        inline void SimplifiedGeometrySink::SetSegmentFlags(PathSegment flags) const
        {
            (*this)->SetSegmentFlags(static_cast<D2D1_PATH_SEGMENT>(flags));
        }

        inline void SimplifiedGeometrySink::BeginFigure(Point2F const & startPoint,
                                                        FigureBegin figureBegin) const
        {
            (*this)->BeginFigure(startPoint.Ref(),
                                 static_cast<D2D1_FIGURE_BEGIN>(figureBegin));
        }

        inline void SimplifiedGeometrySink::AddLines(Point2F const * points,
                                                     unsigned count) const
        {
            ASSERT(points);
            ASSERT(count);

            (*this)->AddLines(points->Get(),
                                count);
        }

        inline void SimplifiedGeometrySink::AddBeziers(BezierSegment const * beziers,
                                                       unsigned count) const
        {
            ASSERT(beziers);
            ASSERT(count);

            (*this)->AddBeziers(beziers->Get(),
                                count);
        }

        inline void SimplifiedGeometrySink::EndFigure(FigureEnd figureEnd) const
        {
            (*this)->EndFigure(static_cast<D2D1_FIGURE_END>(figureEnd));
        }

        inline void TessellationSink::AddTriangles(Triangle const * triangles,
                                                   unsigned count) const
        {
            ASSERT(triangles);
            ASSERT(count);

            (*this)->AddTriangles(triangles->Get(),
                                    count);
        }

        inline void TessellationSink::Close()
        {
            HR((*this)->Close());
        }

        inline auto Resource::GetFactory() const -> Factory
        {
            Factory result;
            (*this)->GetFactory(result.GetAddressOf());
            return result;
        }

        inline auto Bitmap::GetSize() const -> SizeF
        {
            return (*this)->GetSize();
        }

        inline auto Bitmap::GetPixelSize() const -> SizeU
        {
            return (*this)->GetPixelSize();
        }

        inline auto Bitmap::GetPixelFormat() const -> PixelFormat
        {
            return (*this)->GetPixelFormat();
        }

        inline void Bitmap::GetDpi(float & x, float & y) const
        {
            (*this)->GetDpi(&x, &y);
        }

        inline void Bitmap::CopyFromBitmap(Bitmap const & other) const
        {
            HR((*this)->CopyFromBitmap(nullptr,
                                       other.Get(),
                                       nullptr));
        }

        inline void Bitmap::CopyFromBitmap(Bitmap const & other,
                                           Point2U const & destination) const
        {
            HR((*this)->CopyFromBitmap(destination.Get(),
                                       other.Get(),
                                       nullptr));
        }

        inline void Bitmap::CopyFromBitmap(Bitmap const & other,
                                           RectU const & source) const
        {
            HR((*this)->CopyFromBitmap(nullptr,
                                       other.Get(),
                                       source.Get()));
        }

        inline void Bitmap::CopyFromBitmap(Bitmap const & other,
                                           Point2U const & destination,
                                           RectU const & source) const
        {
            HR((*this)->CopyFromBitmap(destination.Get(),
                                       other.Get(),
                                       source.Get()));
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

        inline void Bitmap::CopyFromMemory(void const * data,
                                           unsigned pitch) const
        {
            HR((*this)->CopyFromMemory(nullptr,
                                       data,
                                       pitch));
        }

        inline void Bitmap::CopyFromMemory(void const * data,
                                           unsigned pitch,
                                           RectU const & destination) const
        {
            HR((*this)->CopyFromMemory(destination.Get(),
                                       data,
                                       pitch));
        }

        inline auto ColorContext::GetColorSpace() const -> ColorSpace
        {
            return static_cast<ColorSpace>((*this)->GetColorSpace());
        }

        inline auto ColorContext::GetProfileSize() const -> unsigned
        {
            return (*this)->GetProfileSize();
        }

        inline void ColorContext::GetProfile(BYTE * profile,
                                             unsigned size) const
        {
            HR((*this)->GetProfile(profile,
                                   size));
        }

        inline auto Bitmap1::GetColorContext() const -> ColorContext
        {
            ColorContext result;
            (*this)->GetColorContext(result.GetAddressOf());
            return result;
        }

        inline auto Bitmap1::GetOptions() const -> BitmapOptions
        {
            return static_cast<BitmapOptions>((*this)->GetOptions());
        }

        inline auto Bitmap1::GetSurface() const -> Dxgi::Surface
        {
            Dxgi::Surface result;
            HR((*this)->GetSurface(result.GetAddressOf()));
            return result;
        }

        inline void Bitmap1::Map(MapOptions options,
                                 MappedRect & mappedRect) const
        {
            HR((*this)->Map(static_cast<D2D1_MAP_OPTIONS>(options),
                            mappedRect.Get()));
        }

        inline void Bitmap1::Unmap() const
        {
            HR((*this)->Unmap());
        }

        inline auto GradientStopCollection::GetGradientStopCount() const -> unsigned
        {
            return (*this)->GetGradientStopCount();
        }

        inline void GradientStopCollection::GetGradientStops(GradientStop * stops,
                                                             unsigned count) const
        {
            ASSERT(stops);
            ASSERT(count);

            (*this)->GetGradientStops(stops->Get(),
                                      count);
        }

        inline auto GradientStopCollection::GetColorInterpolationGamma() const -> Gamma
        {
            return static_cast<Gamma>((*this)->GetColorInterpolationGamma());
        }

        inline auto GradientStopCollection::GetExtendMode() const -> ExtendMode
        {
            return static_cast<ExtendMode>((*this)->GetExtendMode());
        }

        inline void GradientStopCollection1::GetGradientStops1(GradientStop * stops,
                                                               unsigned count) const
        {
            ASSERT(stops);
            ASSERT(count);

            (*this)->GetGradientStops1(stops->Get(),
                                       count);
        }

        inline auto GradientStopCollection1::GetPreInterpolationSpace() const -> ColorSpace
        {
            return static_cast<ColorSpace>((*this)->GetPreInterpolationSpace());
        }

        inline auto GradientStopCollection1::GetPostInterpolationSpace() const -> ColorSpace
        {
            return static_cast<ColorSpace>((*this)->GetPostInterpolationSpace());
        }

        inline auto GradientStopCollection1::GetBufferPrecision() const -> BufferPrecision
        {
            return static_cast<BufferPrecision>((*this)->GetBufferPrecision());
        }

        inline auto GradientStopCollection1::GetColorInterpolationMode() const -> ColorInterpolationMode
        {
            return static_cast<ColorInterpolationMode>((*this)->GetColorInterpolationMode());
        }

        inline void Brush::SetOpacity(float opacity) const
        {
            (*this)->SetOpacity(opacity);
        }

        inline auto Brush::GetOpacity() const -> float
        {
            return (*this)->GetOpacity();
        }

        inline void Brush::GetTransform(D2D1_MATRIX_3X2_F & transform) const
        {
            (*this)->GetTransform(&transform);
        }

        inline void Brush::SetTransform(D2D1_MATRIX_3X2_F const & transform) const
        {
            (*this)->SetTransform(transform);
        }

        inline void BitmapBrush::SetExtendModeX(ExtendMode mode) const
        {
            (*this)->SetExtendModeX(static_cast<D2D1_EXTEND_MODE>(mode));
        }

        inline void BitmapBrush::SetExtendModeY(ExtendMode mode) const
        {
            (*this)->SetExtendModeY(static_cast<D2D1_EXTEND_MODE>(mode));
        }

        inline void BitmapBrush::SetInterpolationMode(BitmapInterpolationMode mode) const
        {
            (*this)->SetInterpolationMode(static_cast<D2D1_BITMAP_INTERPOLATION_MODE>(mode));
        }

        inline void BitmapBrush::SetBitmap(Bitmap const & bitmap) const
        {
            (*this)->SetBitmap(bitmap.Get());
        }

        inline auto BitmapBrush::GetExtendModeX() const -> ExtendMode
        {
            return static_cast<ExtendMode>((*this)->GetExtendModeX());
        }

        inline auto BitmapBrush::GetExtendModeY() const -> ExtendMode
        {
            return static_cast<ExtendMode>((*this)->GetExtendModeY());
        }

        inline auto BitmapBrush::GetInterpolationMode() const -> BitmapInterpolationMode
        {
            return static_cast<BitmapInterpolationMode>((*this)->GetInterpolationMode());
        }

        inline auto BitmapBrush::GetBitmap() const -> Bitmap
        {
            Bitmap result;
            (*this)->GetBitmap(result.GetAddressOf());
            return result;
        }

        inline void BitmapBrush1::SetInterpolationMode1(InterpolationMode mode) const
        {
            (*this)->SetInterpolationMode1(static_cast<D2D1_INTERPOLATION_MODE>(mode));
        }

        inline auto BitmapBrush1::GetInterpolationMode1() const -> InterpolationMode
        {
            return static_cast<InterpolationMode>((*this)->GetInterpolationMode1());
        }

        inline void SolidColorBrush::SetColor(Color const & color) const
        {
            (*this)->SetColor(color.Get());
        }

        inline auto SolidColorBrush::GetColor() const -> Color
        {
            return (*this)->GetColor();
        }

        inline void LinearGradientBrush::SetStartPoint(Point2F const & point) const
        {
            (*this)->SetStartPoint(point.Ref());
        }

        inline void LinearGradientBrush::SetEndPoint(Point2F const & point) const
        {
            (*this)->SetEndPoint(point.Ref());
        }

        inline auto LinearGradientBrush::GetStartPoint() const -> Point2F
        {
            return (*this)->GetStartPoint();
        }

        inline auto LinearGradientBrush::GetEndPoint() const -> Point2F
        {
            return (*this)->GetEndPoint();
        }

        inline auto LinearGradientBrush::GetGradientStopCollection() const -> GradientStopCollection
        {
            GradientStopCollection result;
            (*this)->GetGradientStopCollection(result.GetAddressOf());
            return result;
        };

        inline void RadialGradientBrush::SetCenter(Point2F const & point) const
        {
            (*this)->SetCenter(point.Ref());
        }

        inline void RadialGradientBrush::SetGradientOriginOffset(Point2F const & point) const
        {
            (*this)->SetGradientOriginOffset(point.Ref());
        }

        inline void RadialGradientBrush::SetRadiusX(float radius) const
        {
            (*this)->SetRadiusX(radius);
        }

        inline void RadialGradientBrush::SetRadiusY(float radius) const
        {
            (*this)->SetRadiusY(radius);
        }

        inline auto RadialGradientBrush::GetCenter() const -> Point2F
        {
            return (*this)->GetCenter();
        }

        inline auto RadialGradientBrush::GetGradientOriginOffset() const -> Point2F
        {
            return (*this)->GetGradientOriginOffset();
        }

        inline auto RadialGradientBrush::GetRadiusX() const -> float
        {
            return (*this)->GetRadiusX();
        }

        inline auto RadialGradientBrush::GetRadiusY() const -> float
        {
            return (*this)->GetRadiusY();
        }

        inline auto RadialGradientBrush::GetGradientStopCollection() const -> GradientStopCollection
        {
            GradientStopCollection result;
            (*this)->GetGradientStopCollection(result.GetAddressOf());
            return result;
        }

        inline auto StrokeStyle::GetStartCap() const -> CapStyle
        {
            return static_cast<CapStyle>((*this)->GetStartCap());
        }

        inline auto StrokeStyle::GetEndCap() const -> CapStyle
        {
            return static_cast<CapStyle>((*this)->GetEndCap());
        }
        
        inline auto StrokeStyle::GetDashCap() const -> CapStyle
        {
            return static_cast<CapStyle>((*this)->GetDashCap());
        }

        inline auto StrokeStyle::GetMiterLimit() const -> float
        {
            return (*this)->GetMiterLimit();
        }

        inline auto StrokeStyle::GetLineJoin() const -> LineJoin
        {
            return static_cast<LineJoin>((*this)->GetLineJoin());
        }

        inline auto StrokeStyle::GetDashOffset() const -> float
        {
            return (*this)->GetDashOffset();
        }

        inline auto StrokeStyle::GetDashStyle() const -> DashStyle
        {
            return static_cast<DashStyle>((*this)->GetDashStyle());
        }

        inline auto StrokeStyle::GetDashesCount() const -> unsigned
        {
            return (*this)->GetDashesCount();
        }

        inline void StrokeStyle::GetDashes(float * dashes,
                                           unsigned count) const
        {
            (*this)->GetDashes(dashes,
                               count);
        }

        inline auto StrokeStyle1::GetStrokeTransformType() const -> StrokeTransformType
        {
            static_cast<StrokeTransformType>((*this)->GetStrokeTransformType());
        }
    }

    #pragma endregion Implementation
}
