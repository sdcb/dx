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

        enum class WICDecodeCacheOption
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
        struct SampleProperties
        {
            KENNYKERR_DEFINE_STRUCT(SampleProperties, DXGI_SAMPLE_DESC)

            explicit SampleProperties(unsigned const count   = 1,
                                      unsigned const quality = 0) :
                Count(count),
                Quality(quality)
            {}

            unsigned Count;
            unsigned Quality;
        };

        struct SwapChainProperties
        {
            KENNYKERR_DEFINE_STRUCT(SwapChainProperties, DXGI_SWAP_CHAIN_DESC1)

            explicit SwapChainProperties(Format const format         = Format::B8G8R8A8_UNORM,
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
            SampleProperties Sample;
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
        struct TextureProperties2D
        {
            KENNYKERR_DEFINE_STRUCT(TextureProperties2D, D3D11_TEXTURE2D_DESC)

            explicit TextureProperties2D(Dxgi::Format const format          = Dxgi::Format::B8G8R8A8_UNORM,
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
            Dxgi::SampleProperties Sample;
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

}
