#pragma once

#include <EngineFactoryVk.h>
#include <PipelineState.h>
#include <RefCntAutoPtr.hpp>
#include <RenderDevice.h>
#include <Texture.h>

#include <ncore/kernel/collection.h>
#include <ncore/modules/video/renderer.h>

#include "diligent_allocator.h"

namespace nc {

/**
 * @brief VkRenderer is a Vulkan implementation of IRenderer.
 */
class VkRenderer : public IRenderer {
    NCLASS( VkRenderer, IRenderer )

public:
    VkRenderer();
    ~VkRenderer() override;

    VkRenderer( const VkRenderer& )            = delete;
    VkRenderer& operator=( const VkRenderer& ) = delete;

    Ref<IRenderSurface> create_surface( void* native_whnd, Vec2 size ) override;

    RID create_texture( uint32_t w, uint32_t h, const void* pixels ) override;
    RID create_pipeline( const PipelineDesc& desc ) override;
    RID create_buffer( const BufferDesc& desc ) override;
    void destroy_resource( RID rid ) override;

    void* get_native_texture_view( RID rid ) override;
    void* get_native_handle() const override;

    RID get_white_texture() const override
    {
        return white_texture_rid;
    }

    void render_2d( IRenderSurface& target ) override;
    void render_3d( IRenderSurface& target ) override;

    void batch_push_quad( RID texture, Vec4 dest, Vec4 src, Color tint ) override;
    void batch_push_indexed(
        const void* vertices, uint32_t vertex_count, const uint16_t* indices, uint32_t index_count, RID texture,
        Vec4 clip_rect
    ) override;

private:
    struct PipelineEntry {
        Diligent::RefCntAutoPtr<Diligent::IPipelineState> State;
        Diligent::RefCntAutoPtr<Diligent::IShaderResourceBinding> ShaderResBinding;
        Diligent::IShaderResourceVariable* TextureVar = nullptr;
    };

    struct DiligentVertexFormat {
        Diligent::Uint32 num_components;
        Diligent::VALUE_TYPE value_type;
        Diligent::Bool is_normalized;
    };

    DiligentVertexFormat translate_vertex_attrib_type( VertexAttribType type );
    Vector<Diligent::LayoutElement> translate_vertex_layout( std::span<const VertexAttribute> attribs );
    Diligent::SHADER_TYPE translate_stage( ShaderStage s );
    Diligent::SHADER_RESOURCE_VARIABLE_TYPE translate_var_kind( ResourceVarKind k );
    Diligent::CULL_MODE translate_cull( CullMode c );
    void apply_blend_preset( Diligent::RenderTargetBlendDesc& rt, BlendPreset preset );

    NcoreDiligentAllocator allocator;

    Diligent::RefCntAutoPtr<Diligent::IEngineFactoryVk> engine_factory;
    Diligent::RefCntAutoPtr<Diligent::IRenderDevice> render_device;
    Diligent::RefCntAutoPtr<Diligent::IDeviceContext> device_ctx;
    Diligent::RefCntAutoPtr<Diligent::IShaderSourceInputStreamFactory> shader_src_factory;

    PagedResourcePool<PipelineEntry> pipeline_cache;
    PagedResourcePool<Diligent::RefCntAutoPtr<Diligent::ITexture>> texture_cache;
    PagedResourcePool<Diligent::RefCntAutoPtr<Diligent::IBuffer>> buffer_cache;

    Diligent::RefCntAutoPtr<Diligent::ITexture> white_texture;
    Diligent::ITextureView* white_tex_view = nullptr;
    RID white_texture_rid;

    struct BatchDrawCmd {
        std::vector<uint8_t> vertices;
        std::vector<uint16_t> indices;
        void* native_texture = nullptr;
        Vec4 clip_rect;
        bool is_textured = false;
    };

    void ensure_batched_2d_buffers_( size_t needed_vb, size_t needed_ib );
    static constexpr size_t VERTEX_STRIDE = sizeof( Vertex2D );
    RID batched_2d_pipeline;
    RID ortho_proj;
    RID batched_2d_vb;
    RID batched_2d_ib;
    Diligent::Uint32 batched_2d_vb_capacity = 0;
    Diligent::Uint32 batched_2d_ib_capacity = 0;
    std::vector<BatchDrawCmd> m_cmds;
};

} // namespace nc
