#pragma once

#include <ncore/kernel/object.h>
#include <ncore/kernel/reference.h>
#include <ncore/kernel/rid.h>
#include <ncore/kernel/structures.h>

namespace nc {

class IRenderSurface;

/**
 * @brief IRenderer owns GPU resources and drives rendering.
 * It contains all resource management and phase dispatch in one place.
 */
class IRenderer : public NcObject {
    NCLASS( IRenderer, NcObject )

public:
    enum class BufferType {
        Vertex,
        Index,
        Uniform
    };

    enum class VertexAttribType {
        Float2,
        Float3,
        Float4,
        UByte4Norm,
    };

    struct VertexAttribute {
        VertexAttribType type;
    };

    enum class ShaderStage {
        Vertex,
        Pixel,
    };

    enum class ResourceVarKind {
        Static,
        Mutable,
        Dynamic,
    };

    struct ResourceBinding {
        ShaderStage stage;
        std::string_view name;
        ResourceVarKind kind;
    };

    enum class CullMode {
        None,
        Front,
        Back,
    };

    enum class BlendPreset {
        Opaque,
        AlphaBlend,
        Additive,
    };

	struct BufferDesc {
        std::string debug_name;
        BufferType type;
        size_t size;
        const void* data;
        bool dynamic;
	};

    struct PipelineDesc {
        std::string debug_name;
        std::string_view vs_spirv;
        std::string_view ps_spirv;
        std::span<const VertexAttribute> vertex_attribs;
        std::span<const ResourceBinding> bindings;
        CullMode cull_mode = CullMode::None;
        bool scissor_cull  = false;
        bool depth_test    = false;
        bool depth_write   = false;
        BlendPreset blend  = BlendPreset::AlphaBlend;
    };

public:
    // RESOURCES

    /**
     * @brief Creates a per-window presentation surface (swap chain).
     *
     * @param native_whnd Platform-native window handle (e.g. HWND).
     * @param size        Initial surface dimensions.
     */
    virtual Ref<IRenderSurface> create_surface( void* native_whnd, Vec2 size ) = 0;

    virtual RID create_texture( uint32_t w, uint32_t h, const void* pixels )                  = 0;
    virtual RID create_pipeline( const PipelineDesc& desc )                                   = 0;
    virtual RID create_buffer( const BufferDesc& desc ) = 0;
    virtual void destroy_resource( RID rid )                                                  = 0;

    virtual void* get_native_texture_view( RID rid ) = 0;
    virtual void* get_native_handle() const          = 0;

    virtual RID get_white_texture() const = 0;

    // RENDERING

    virtual void render_2d( IRenderSurface& target ) = 0;
    virtual void render_3d( IRenderSurface& target ) = 0;

    virtual void batch_push_quad( RID texture, Vec4 dest, Vec4 src, Color tint ) = 0;
    virtual void batch_push_indexed(
        const void* vertices, uint32_t vertex_count, const uint16_t* indices, uint32_t index_count, RID texture,
        Vec4 clip_rect
    ) = 0;
};

} // namespace nc
