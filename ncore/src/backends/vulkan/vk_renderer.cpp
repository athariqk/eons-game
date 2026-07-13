#include "vk_renderer.h"

#include <BasicMath.hpp>
#include <BasicTypes.h>
#include <DebugOutput.h>
#include <EngineMemory.h>
#include <GraphicsAccessories.hpp>
#include <GraphicsTypes.h>
#include <MapHelper.hpp>
#include <format>
#include <iterator>

#include <ncore/utils/assert.h>
#include <ncore/utils/log.h>

#include "vk_render_surface.h"

#include <assets/global_res.h>

namespace nc {

static void DILIGENT_CALL_TYPE DebugMessageCallbackVk(
    enum Diligent::DEBUG_MESSAGE_SEVERITY Severity, const Diligent::Char* Message, const Diligent::Char* Function,
    const Diligent::Char* File, int Line
)
{
    switch (Severity) {
        case Diligent::DEBUG_MESSAGE_SEVERITY_INFO:
            NC_LOG( log::GRAPHICS, 2, File, Function, Line, "Vulkan: {}", Message );
            break;
        case Diligent::DEBUG_MESSAGE_SEVERITY_WARNING:
            NC_LOG( log::GRAPHICS, 3, File, Function, Line, "Vulkan: {}", Message );
            break;
        case Diligent::DEBUG_MESSAGE_SEVERITY_ERROR:
            NC_LOG( log::GRAPHICS, 4, File, Function, Line, "Vulkan: {}", Message );
            break;
        case Diligent::DEBUG_MESSAGE_SEVERITY_FATAL_ERROR:
            NC_LOG( log::GRAPHICS, 5, File, Function, Line, "Vulkan: {}", Message );
            break;
    }
}

VkRenderer::VkRenderer()
{
    NC_LOG_TRACE_C( log::GRAPHICS, "Initializing VkRenderer" );

    Diligent::SetRawAllocator( &allocator );

    engine_factory = Diligent::LoadAndGetEngineFactoryVk();
    engine_factory->SetMessageCallback( DebugMessageCallbackVk );

    auto vk_version = engine_factory->GetVulkanVersion();
    NC_LOG_INFO_C( log::GRAPHICS, "Vulkan version: {}.{}", vk_version.Major, vk_version.Minor );

    Diligent::EngineVkCreateInfo engine_ci;
    engine_factory->CreateDeviceAndContextsVk( engine_ci, &render_device, &device_ctx );
    NC_ASSERT( render_device && device_ctx, "Failed to create Vulkan device and contexts" );

    engine_factory->CreateDefaultShaderSourceStreamFactory( nullptr, &shader_src_factory );

    // Create 2D pipeline
    {
        static constexpr VertexAttribute s_vertex_2d_attribs[] = {
            { VertexAttribType::Float2 },     // position
            { VertexAttribType::Float2 },     // uv
            { VertexAttribType::UByte4Norm }, // color
        };

        static const ResourceBinding s_2d_bindings[] = { { ShaderStage::Pixel, "Texture", ResourceVarKind::Dynamic } };

        const bool srgb = Diligent::GetTextureFormatAttribs( Diligent::TEX_FORMAT_RGBA8_UNORM_SRGB ).ComponentType ==
                          Diligent::COMPONENT_TYPE_UNORM_SRGB;
        const auto* ps_data = srgb ? GlobalRes::SPRITE_FRAGMENT_GAMMA_SPIRV : GlobalRes::SPRITE_FRAGMENT_SPIRV;
        const auto ps_size =
            srgb ? sizeof( GlobalRes::SPRITE_FRAGMENT_GAMMA_SPIRV ) : sizeof( GlobalRes::SPRITE_FRAGMENT_SPIRV );

        PipelineDesc desc{
            .debug_name = "2D Batch Pipeline",
            .vs_spirv   = std::string_view(
                reinterpret_cast<const char*>( GlobalRes::SPRITE_VERTEX_SPIRV ),
                sizeof( GlobalRes::SPRITE_VERTEX_SPIRV )
            ),
            .ps_spirv       = std::string_view( reinterpret_cast<const char*>( ps_data ), ps_size ),
            .vertex_attribs = s_vertex_2d_attribs,
            .bindings       = s_2d_bindings,
            .cull_mode      = CullMode::None,
            .blend          = BlendPreset::AlphaBlend,
        };

        batched_2d_pipeline = create_pipeline( desc );
        auto pipeline       = pipeline_cache.get( batched_2d_pipeline );

        // Constant buffer for ortho projection
        ortho_proj = create_buffer(
            BufferDesc{
                .debug_name = "Ortho Projection",
                .type       = BufferType::Uniform,
                .size       = sizeof( Diligent::float4x4 ),
                .data       = nullptr,
                .dynamic    = true
            }
        );
        auto ortho_proj_buf = buffer_cache.get( ortho_proj );
        auto* constants_var = pipeline->State->GetStaticVariableByName( Diligent::SHADER_TYPE_VERTEX, "Constants" );
        if (constants_var)
            constants_var->Set( *ortho_proj_buf );

        pipeline->State->CreateShaderResourceBinding( &pipeline->ShaderResBinding, true );
        pipeline->TextureVar = pipeline->ShaderResBinding->GetVariableByName( Diligent::SHADER_TYPE_PIXEL, "Texture" );

        // 1x1 white texture (shared, registered in the texture cache so it has a RID)
        {
            const uint32_t white = 0xFFFFFFFF;
            white_texture_rid    = create_texture( 1, 1, &white );
            white_tex_view       = static_cast<Diligent::ITextureView*>( get_native_texture_view( white_texture_rid ) );
        }
    }
}

VkRenderer::~VkRenderer()
{
    NC_LOG_TRACE_C( log::GRAPHICS, "Shutting down VkRenderer" );
}

//------------------------------------------------------------------------------

Ref<IRenderSurface> VkRenderer::create_surface( void* native_handle, Vec2 size )
{
    return Ref<VkRenderSurface>::create(
        native_handle, size, engine_factory.RawPtr(), render_device.RawPtr(), device_ctx.RawPtr()
    );
}

RID VkRenderer::create_texture( uint32_t w, uint32_t h, const void* pixels )
{
    RID handle = texture_cache.acquire();

    auto tex_name = std::format( "ncore_texture_2d_{}", handle.value );
    Diligent::TextureDesc desc;
    desc.Name      = tex_name.c_str();
    desc.Type      = Diligent::RESOURCE_DIM_TEX_2D;
    desc.Width     = static_cast<Diligent::Uint32>( w );
    desc.Height    = static_cast<Diligent::Uint32>( h );
    desc.Format    = Diligent::TEX_FORMAT_RGBA8_UNORM;
    desc.Usage     = Diligent::USAGE_DEFAULT;
    desc.BindFlags = Diligent::BIND_SHADER_RESOURCE;

    Diligent::TextureSubResData mip_0{ pixels, static_cast<Diligent::Uint64>( w * 4 ) };
    Diligent::TextureData init{ &mip_0, 1 };

    Diligent::RefCntAutoPtr<Diligent::ITexture> texture;
    render_device->CreateTexture( desc, &init, &texture );

    auto* entry = texture_cache.get( handle );
    if (entry) {
        *entry = std::move( texture );
    }

    return handle;
}

RID VkRenderer::create_pipeline( const PipelineDesc& desc )
{
    Diligent::RefCntAutoPtr<Diligent::IShader> vs;
    {
        auto name = desc.debug_name + "_VS";
        Diligent::ShaderCreateInfo ci;
        ci.Desc         = { name.c_str(), Diligent::SHADER_TYPE_VERTEX, true };
        ci.ByteCode     = desc.vs_spirv.data();
        ci.ByteCodeSize = desc.vs_spirv.size();

        render_device->CreateShader( ci, &vs );
    }

    Diligent::RefCntAutoPtr<Diligent::IShader> ps;
    {
        auto name = desc.debug_name + "_PS";
        Diligent::ShaderCreateInfo ci;
        ci.Desc         = { name.c_str(), Diligent::SHADER_TYPE_PIXEL, true };
        ci.ByteCode     = desc.ps_spirv.data();
        ci.ByteCodeSize = desc.ps_spirv.size();

        render_device->CreateShader( ci, &ps );
    }

    Diligent::RefCntAutoPtr<Diligent::IPipelineState> pso;
    {
        Diligent::GraphicsPipelineStateCreateInfo ci;
        ci.PSODesc.Name         = desc.debug_name.c_str();
        ci.PSODesc.PipelineType = Diligent::PIPELINE_TYPE_GRAPHICS;
        ci.pVS                  = vs;
        ci.pPS                  = ps;

        ci.GraphicsPipeline.NumRenderTargets             = 1;
        ci.GraphicsPipeline.RTVFormats[0]                = Diligent::TEX_FORMAT_RGBA8_UNORM_SRGB;
        ci.GraphicsPipeline.DSVFormat                    = Diligent::TEX_FORMAT_D32_FLOAT;
        ci.GraphicsPipeline.PrimitiveTopology            = Diligent::PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
        ci.GraphicsPipeline.RasterizerDesc.CullMode      = translate_cull( desc.cull_mode );
        ci.GraphicsPipeline.RasterizerDesc.ScissorEnable = static_cast<Diligent::Bool>( desc.scissor_cull );
        ci.GraphicsPipeline.DepthStencilDesc.DepthEnable = static_cast<Diligent::Bool>( desc.depth_test );

        auto inputs                                    = translate_vertex_layout( desc.vertex_attribs );
        ci.GraphicsPipeline.InputLayout.NumElements    = static_cast<Diligent::Uint32>( std::size( inputs ) );
        ci.GraphicsPipeline.InputLayout.LayoutElements = inputs.data();

        Vector<Diligent::ShaderResourceVariableDesc> var_descs;
        var_descs.reserve( desc.bindings.size() );
        for (const auto& b : desc.bindings) {
            var_descs.push_back( { translate_stage( b.stage ), b.name.data(), translate_var_kind( b.kind ) } );
        }
        ci.PSODesc.ResourceLayout.Variables           = var_descs.data();
        ci.PSODesc.ResourceLayout.NumVariables        = static_cast<Diligent::Uint32>( var_descs.size() );
        ci.PSODesc.ResourceLayout.DefaultVariableType = Diligent::SHADER_RESOURCE_VARIABLE_TYPE_STATIC;

        Diligent::RenderTargetBlendDesc& rt = ci.GraphicsPipeline.BlendDesc.RenderTargets[0];
        apply_blend_preset( rt, desc.blend );

        Diligent::SamplerDesc sampler;
        sampler.AddressU                               = Diligent::TEXTURE_ADDRESS_WRAP;
        sampler.AddressV                               = Diligent::TEXTURE_ADDRESS_WRAP;
        sampler.AddressW                               = Diligent::TEXTURE_ADDRESS_WRAP;
        Diligent::ImmutableSamplerDesc immut[]         = { { Diligent::SHADER_TYPE_PIXEL, "Texture", sampler } };
        ci.PSODesc.ResourceLayout.ImmutableSamplers    = immut;
        ci.PSODesc.ResourceLayout.NumImmutableSamplers = std::size( immut );

        render_device->CreateGraphicsPipelineState( ci, &pso );
    }

    NC_ASSERT( vs && ps && pso, "Failed to create pipeline resources" );

    RID handle = pipeline_cache.acquire();
    auto entry = pipeline_cache.get( handle );
    NC_ASSERT( entry, "Failed to acquire new pipeline object" );

    entry->State = pso;

    return handle;
}

RID VkRenderer::create_buffer( const BufferDesc& p_desc )
{
    Diligent::BufferDesc desc;
    desc.Name = p_desc.debug_name.c_str();
    desc.Size = static_cast<Diligent::Uint32>( p_desc.size );

    switch (p_desc.type) {
        case BufferType::Vertex:
            desc.BindFlags = Diligent::BIND_VERTEX_BUFFER;
            break;
        case BufferType::Index:
            desc.BindFlags = Diligent::BIND_INDEX_BUFFER;
            break;
        case BufferType::Uniform:
            desc.BindFlags = Diligent::BIND_UNIFORM_BUFFER;
            break;
    }

    if (p_desc.dynamic) {
        desc.Usage          = Diligent::USAGE_DYNAMIC;
        desc.CPUAccessFlags = Diligent::CPU_ACCESS_WRITE;
    } else {
        desc.Usage = Diligent::USAGE_DEFAULT;
    }

    Diligent::BufferData init_data{ p_desc.data, static_cast<Diligent::Uint32>( p_desc.size ) };
    Diligent::RefCntAutoPtr<Diligent::IBuffer> buffer;
    render_device->CreateBuffer( desc, p_desc.data ? &init_data : nullptr, &buffer );

    RID handle = buffer_cache.acquire();
    auto entry = buffer_cache.get( handle );
    if (entry) {
        *entry = std::move( buffer );
    }

    return handle;
}

void VkRenderer::destroy_resource( RID rid )
{
    if (texture_cache.get( rid )) {
        texture_cache.release( rid );
        return;
    }
    if (pipeline_cache.get( rid )) {
        pipeline_cache.release( rid );
        return;
    }
    if (buffer_cache.get( rid )) {
        buffer_cache.release( rid );
        return;
    }
}

void* VkRenderer::get_native_texture_view( RID rid )
{
    if (auto* t = texture_cache.get( rid )) {
        return ( *t )->GetDefaultView( Diligent::TEXTURE_VIEW_SHADER_RESOURCE );
    }
    return nullptr;
}

void* VkRenderer::get_native_handle() const
{
    return render_device.RawPtr();
}

//------------------------------------------------------------------------------

void VkRenderer::render_2d( IRenderSurface& target )
{
    auto surf_size = target.get_surface_size();

    if (m_cmds.empty())
        return;

    Diligent::Viewport vp;
    vp.TopLeftX = 0.0f;
    vp.TopLeftY = 0.0f;
    vp.Width    = surf_size.X;
    vp.Height   = surf_size.Y;
    vp.MinDepth = 0.0f;
    vp.MaxDepth = 1.0f;
    device_ctx->SetViewports(
        1, &vp, static_cast<Diligent::Uint32>( surf_size.X ), static_cast<Diligent::Uint32>( surf_size.Y )
    );

    // Compute buffer requirements and ensure buffers exist
    size_t total_vb_bytes = 0, total_ib = 0;
    for (auto& cmd : m_cmds) {
        total_vb_bytes += cmd.vertices.size();
        total_ib += cmd.indices.size();
    }

    auto needed_vb = static_cast<Diligent::Uint32>( total_vb_bytes / VERTEX_STRIDE );
    auto needed_ib = static_cast<Diligent::Uint32>( total_ib );
    ensure_batched_2d_buffers_( needed_vb, needed_ib );

    auto pipeline       = pipeline_cache.get( batched_2d_pipeline );
    auto ortho_proj_buf = buffer_cache.get( ortho_proj );
    auto vb             = buffer_cache.get( batched_2d_vb );
    auto ib             = buffer_cache.get( batched_2d_ib );

    if (!pipeline || !ortho_proj_buf || !vb || !ib)
        return;

    // Upload projection matrix
    {
        float L = 0.0f, R = surf_size.X;
        float T = 0.0f, B = surf_size.Y;

        Diligent::float4x4 proj{
            2.0f / ( R - L ),
            0.0f,
            0.0f,
            0.0f,
            0.0f,
            2.0f / ( T - B ),
            0.0f,
            0.0f,
            0.0f,
            0.0f,
            0.5f,
            0.0f,
            ( R + L ) / ( L - R ),
            ( T + B ) / ( B - T ),
            0.5f,
            1.0f
        };

        Diligent::MapHelper<Diligent::float4x4> cb{
            device_ctx, *ortho_proj_buf, Diligent::MAP_WRITE, Diligent::MAP_FLAG_DISCARD
        };
        if (cb)
            *cb = proj;
    }

    // Merge all draws into VB/IB
    {
        Diligent::MapHelper<uint8_t> vb_map{ device_ctx, *vb, Diligent::MAP_WRITE, Diligent::MAP_FLAG_DISCARD };
        Diligent::MapHelper<uint8_t> ib_map{ device_ctx, *ib, Diligent::MAP_WRITE, Diligent::MAP_FLAG_DISCARD };
        if (!vb_map || !ib_map)
            return;

        uint8_t* vb_ptr = vb_map;
        uint8_t* ib_ptr = ib_map;
        for (auto& cmd : m_cmds) {
            memcpy( vb_ptr, cmd.vertices.data(), cmd.vertices.size() );
            vb_ptr += cmd.vertices.size();
            memcpy( ib_ptr, cmd.indices.data(), cmd.indices.size() * sizeof( uint16_t ) );
            ib_ptr += cmd.indices.size() * sizeof( uint16_t );
        }
    }

    // Set immutable state once before the draw loop
    Diligent::IBuffer* vb_arr[] = { *vb };
    device_ctx->SetVertexBuffers(
        0, 1, vb_arr, nullptr, Diligent::RESOURCE_STATE_TRANSITION_MODE_TRANSITION,
        Diligent::SET_VERTEX_BUFFERS_FLAG_RESET
    );
    device_ctx->SetIndexBuffer( *ib, 0, Diligent::RESOURCE_STATE_TRANSITION_MODE_TRANSITION );
    device_ctx->SetPipelineState( pipeline->State );

    Diligent::Rect full_scissor{
        0, 0, static_cast<Diligent::Int32>( surf_size.X ), static_cast<Diligent::Int32>( surf_size.Y )
    };
    device_ctx->SetScissorRects(
        1, &full_scissor, static_cast<Diligent::Uint32>( surf_size.X ), static_cast<Diligent::Uint32>( surf_size.Y )
    );

    Diligent::Uint32 vb_offset = 0;
    Diligent::Uint32 ib_offset = 0;
    bool was_clipped           = false;
    void* last_tex             = nullptr;

    for (auto& cmd : m_cmds) {
        if (cmd.indices.empty())
            continue;

        if (!cmd.clip_rect.is_zero()) {
            Diligent::Rect scissor{
                static_cast<Diligent::Int32>( cmd.clip_rect.X ), static_cast<Diligent::Int32>( cmd.clip_rect.Y ),
                static_cast<Diligent::Int32>( cmd.clip_rect.X + cmd.clip_rect.w ),
                static_cast<Diligent::Int32>( cmd.clip_rect.Y + cmd.clip_rect.h )
            };
            scissor.left   = std::max( scissor.left, 0 );
            scissor.top    = std::max( scissor.top, 0 );
            scissor.right  = std::min( scissor.right, static_cast<Diligent::Int32>( surf_size.X ) );
            scissor.bottom = std::min( scissor.bottom, static_cast<Diligent::Int32>( surf_size.Y ) );
            if (scissor.IsValid()) {
                device_ctx->SetScissorRects(
                    1, &scissor, static_cast<Diligent::Uint32>( surf_size.X ),
                    static_cast<Diligent::Uint32>( surf_size.Y )
                );
                was_clipped = true;
            }
        } else if (was_clipped) {
            device_ctx->SetScissorRects(
                1, &full_scissor, static_cast<Diligent::Uint32>( surf_size.X ),
                static_cast<Diligent::Uint32>( surf_size.Y )
            );
            was_clipped = false;
        }

        auto* tex_view = static_cast<Diligent::ITextureView*>( cmd.native_texture );
        if (pipeline->TextureVar && tex_view && tex_view != last_tex) {
            pipeline->TextureVar->Set( tex_view );
            device_ctx->CommitShaderResources(
                pipeline->ShaderResBinding, Diligent::RESOURCE_STATE_TRANSITION_MODE_TRANSITION
            );
            last_tex = tex_view;
        }

        Diligent::DrawIndexedAttribs attrs{
            static_cast<Diligent::Uint32>( cmd.indices.size() ), Diligent::VT_UINT16, Diligent::DRAW_FLAG_NONE
        };
        attrs.FirstIndexLocation = ib_offset;
        attrs.BaseVertex         = vb_offset;
        device_ctx->DrawIndexed( attrs );

        vb_offset += static_cast<Diligent::Uint32>( cmd.vertices.size() / VERTEX_STRIDE );
        ib_offset += static_cast<Diligent::Uint32>( cmd.indices.size() );
    }

    m_cmds.clear();
}

void VkRenderer::render_3d( IRenderSurface& target ) {}

static void append_vertex( std::vector<uint8_t>& buf, float X, float Y, float u, float v, uint32_t color )
{
    Vertex2D vert{ X, Y, u, v, color };
    auto* p = reinterpret_cast<const uint8_t*>( &vert );
    buf.insert( buf.end(), p, p + sizeof( Vertex2D ) );
}

//------------------------------------------------------------------------------

static void add_quad(
    std::vector<uint8_t>& verts, std::vector<uint16_t>& idx, float x1, float y1, float x2, float y2, float u1, float v1,
    float u2, float v2, uint32_t color
)
{
    uint16_t base = static_cast<uint16_t>( verts.size() / sizeof( Vertex2D ) );
    append_vertex( verts, x1, y1, u1, v1, color );
    append_vertex( verts, x2, y1, u2, v1, color );
    append_vertex( verts, x2, y2, u2, v2, color );
    append_vertex( verts, x1, y2, u1, v2, color );
    idx.insert(
        idx.end(), { base, static_cast<uint16_t>( base + 1 ), static_cast<uint16_t>( base + 2 ), base,
                     static_cast<uint16_t>( base + 2 ), static_cast<uint16_t>( base + 3 ) }
    );
}

static uint32_t color_to_u32( const Color& c )
{
    return ( static_cast<uint32_t>( c.r ) ) | ( static_cast<uint32_t>( c.g ) << 8 ) |
           ( static_cast<uint32_t>( c.b ) << 16 ) | ( static_cast<uint32_t>( c.a ) << 24 );
}

void VkRenderer::batch_push_quad( RID texture, Vec4 dest, Vec4 src, Color tint )
{
    if (!texture.is_valid()) {
        return;
    }
    void* native = get_native_texture_view( texture );

    BatchDrawCmd cmd;
    cmd.native_texture = native;
    cmd.is_textured    = true;

    uint32_t col = color_to_u32( tint );

    float u1 = src.X, v1 = src.Y, u2 = src.w, v2 = src.h;
    float x1 = dest.X, y1 = dest.Y, x2 = dest.X + dest.w, y2 = dest.Y + dest.h;

    add_quad( cmd.vertices, cmd.indices, x1, y1, x2, y2, u1, v1, u2, v2, col );

    m_cmds.push_back( std::move( cmd ) );
}

void VkRenderer::batch_push_indexed(
    const void* vertices, uint32_t vertex_count, const uint16_t* indices, uint32_t index_count, RID texture,
    Vec4 clip_rect
)
{
    void* native = texture.is_valid() ? get_native_texture_view( texture ) : white_tex_view;

    BatchDrawCmd cmd;
    cmd.native_texture = native;
    cmd.clip_rect      = clip_rect;
    cmd.is_textured    = true;

    auto* v = static_cast<const uint8_t*>( vertices );
    cmd.vertices.assign( v, v + vertex_count * VERTEX_STRIDE );
    cmd.indices.assign( indices, indices + index_count );

    m_cmds.push_back( std::move( cmd ) );
}

//------------------------------------------------------------------------------

VkRenderer::DiligentVertexFormat VkRenderer::translate_vertex_attrib_type( VertexAttribType type )
{
    switch (type) {
        case VertexAttribType::Float2:
            return { 2, Diligent::VT_FLOAT32, Diligent::False };
        case VertexAttribType::Float3:
            return { 3, Diligent::VT_FLOAT32, Diligent::False };
        case VertexAttribType::Float4:
            return { 4, Diligent::VT_FLOAT32, Diligent::False };
        case VertexAttribType::UByte4Norm:
            return { 4, Diligent::VT_UINT8, Diligent::True };
    }
    NC_ASSERT( false, "Unhandled VertexAttribType" );
    return { 0, Diligent::VT_UNDEFINED, Diligent::False };
}

Vector<Diligent::LayoutElement> VkRenderer::translate_vertex_layout( std::span<const VertexAttribute> attribs )
{
    Vector<Diligent::LayoutElement> out;
    out.reserve( attribs.size() );

    for (size_t i = 0; i < attribs.size(); ++i) {
        auto fmt = translate_vertex_attrib_type( attribs[i].type );
        out.push_back(
            Diligent::LayoutElement{
                static_cast<Diligent::Uint32>( i ), // InputIndex
                0,                                  // BufferSlot — single interleaved stream
                fmt.num_components, fmt.value_type, fmt.is_normalized
                // RelativeOffset, Stride left at AUTO
            }
        );
    }

    return out;
}

Diligent::SHADER_TYPE VkRenderer::translate_stage( ShaderStage s )
{
    switch (s) {
        case ShaderStage::Vertex:
            return Diligent::SHADER_TYPE_VERTEX;
        case ShaderStage::Pixel:
            return Diligent::SHADER_TYPE_PIXEL;
    }
    NC_ASSERT( false, "Unhandled ShaderStage" );
    return Diligent::SHADER_TYPE_UNKNOWN;
}

Diligent::SHADER_RESOURCE_VARIABLE_TYPE VkRenderer::translate_var_kind( ResourceVarKind k )
{
    switch (k) {
        case ResourceVarKind::Static:
            return Diligent::SHADER_RESOURCE_VARIABLE_TYPE_STATIC;
        case ResourceVarKind::Mutable:
            return Diligent::SHADER_RESOURCE_VARIABLE_TYPE_MUTABLE;
        case ResourceVarKind::Dynamic:
            return Diligent::SHADER_RESOURCE_VARIABLE_TYPE_DYNAMIC;
    }
    NC_ASSERT( false, "Unhandled ResourceVarKind" );
    return Diligent::SHADER_RESOURCE_VARIABLE_TYPE_STATIC;
}

Diligent::CULL_MODE VkRenderer::translate_cull( CullMode c )
{
    switch (c) {
        case CullMode::None:
            return Diligent::CULL_MODE_NONE;
        case CullMode::Front:
            return Diligent::CULL_MODE_FRONT;
        case CullMode::Back:
            return Diligent::CULL_MODE_BACK;
    }
    NC_ASSERT( false, "Unhandled CullMode" );
    return Diligent::CULL_MODE_NONE;
}

void VkRenderer::apply_blend_preset( Diligent::RenderTargetBlendDesc& rt, BlendPreset preset )
{
    switch (preset) {
        case BlendPreset::Opaque:
            rt.BlendEnable = Diligent::False;
            break;
        case BlendPreset::AlphaBlend:
            rt.BlendEnable    = Diligent::True;
            rt.SrcBlend       = Diligent::BLEND_FACTOR_SRC_ALPHA;
            rt.DestBlend      = Diligent::BLEND_FACTOR_INV_SRC_ALPHA;
            rt.BlendOp        = Diligent::BLEND_OPERATION_ADD;
            rt.SrcBlendAlpha  = Diligent::BLEND_FACTOR_ONE;
            rt.DestBlendAlpha = Diligent::BLEND_FACTOR_INV_SRC_ALPHA;
            rt.BlendOpAlpha   = Diligent::BLEND_OPERATION_ADD;
            break;
        case BlendPreset::Additive:
            rt.BlendEnable    = Diligent::True;
            rt.SrcBlend       = Diligent::BLEND_FACTOR_SRC_ALPHA;
            rt.DestBlend      = Diligent::BLEND_FACTOR_ONE;
            rt.BlendOp        = Diligent::BLEND_OPERATION_ADD;
            rt.SrcBlendAlpha  = Diligent::BLEND_FACTOR_ONE;
            rt.DestBlendAlpha = Diligent::BLEND_FACTOR_ONE;
            rt.BlendOpAlpha   = Diligent::BLEND_OPERATION_ADD;
            break;
    }
    rt.RenderTargetWriteMask = Diligent::COLOR_MASK_ALL;
}

void VkRenderer::ensure_batched_2d_buffers_( size_t needed_vb, size_t needed_ib )
{
    auto nv = static_cast<Diligent::Uint32>( needed_vb );
    auto ni = static_cast<Diligent::Uint32>( needed_ib );

    if (nv > batched_2d_vb_capacity) {
        buffer_cache.release( batched_2d_vb );

        while (batched_2d_vb_capacity < nv)
            batched_2d_vb_capacity = batched_2d_vb_capacity == 0 ? 1024 : batched_2d_vb_capacity * 2;

        batched_2d_vb = create_buffer(
            BufferDesc{
                .debug_name = "2D Batch Vertex Buffer",
                .type       = BufferType::Vertex,
                .size       = batched_2d_vb_capacity * VERTEX_STRIDE,
                .data       = nullptr,
                .dynamic    = true
            }
        );
    }

    if (ni > batched_2d_ib_capacity) {
        buffer_cache.release( batched_2d_ib );

        while (batched_2d_ib_capacity < ni)
            batched_2d_ib_capacity = batched_2d_ib_capacity == 0 ? 2048 : batched_2d_ib_capacity * 2;

        batched_2d_ib = create_buffer(
            BufferDesc{
                .debug_name = "2D Batch Index Buffer",
                .type       = BufferType::Index,
                .size       = batched_2d_ib_capacity * sizeof( uint16_t ),
                .data       = nullptr,
                .dynamic    = true
            }
        );
    }
}

} // namespace nc
