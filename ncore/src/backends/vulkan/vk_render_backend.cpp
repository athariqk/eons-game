#include "vk_render_backend.h"

#include <BasicMath.hpp>
#include <BasicTypes.h>
#include <DebugOutput.h>
#include <EngineFactoryVk.h>
#include <GraphicsAccessories.hpp>
#include <GraphicsTypes.h>
#include <format>
#include <iterator>

#include <ncore/modules/video/window_module.h>
#include <ncore/utils/log.h>

#include "vk_global_shaders.h"

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

VkRenderBackend::VkRenderBackend( IWindowModule* p_windows ) : windows( p_windows ) {}

VkRenderBackend::~VkRenderBackend() {}

Error VkRenderBackend::init()
{
    Diligent::IEngineFactoryVk* factory = Diligent::LoadAndGetEngineFactoryVk();
    factory->SetMessageCallback( DebugMessageCallbackVk );

    auto vk_version = factory->GetVulkanVersion();
    NC_LOG_INFO_C( log::GRAPHICS, "Vulkan version: {}.{}", vk_version.Major, vk_version.Minor );

    Diligent::EngineVkCreateInfo engine_ci;
    factory->CreateDeviceAndContextsVk( engine_ci, &render_device, &device_ctx );

    Diligent::NativeWindow window;
#if defined( _WIN32 )
    window.hWnd = windows->get_native_handle( windows->get_main_window_id() );
#else
    NC_ASSERT( false, "Native handle retrieval not implemented for this platform" );
#endif

    Diligent::SwapChainDesc swap_chain_desc;
    swap_chain_desc.IsPrimary = true;
    factory->CreateSwapChainVk( render_device, device_ctx, swap_chain_desc, window, &swap_chain );

    factory->CreateDefaultShaderSourceStreamFactory( nullptr, &shader_src_factory );

    create_2d_pipeline_state_();

    return Error::OK;
}

void VkRenderBackend::finalize() {}

void VkRenderBackend::begin_frame()
{
    batch2d = std::make_unique<BatchRenderer2D>( render_device, device_ctx, pso_2d, srb_2d, texture_var_2d );

    Diligent::ITextureView* rtv = swap_chain->GetCurrentBackBufferRTV();
    Diligent::ITextureView* dsv = swap_chain->GetDepthBufferDSV();
    device_ctx->SetRenderTargets( 1, &rtv, dsv, Diligent::RESOURCE_STATE_TRANSITION_MODE_TRANSITION );

    device_ctx->ClearRenderTarget( rtv, clear_color, Diligent::RESOURCE_STATE_TRANSITION_MODE_TRANSITION );
    device_ctx->ClearDepthStencil(
        dsv, Diligent::CLEAR_DEPTH_FLAG, 1.f, 0, Diligent::RESOURCE_STATE_TRANSITION_MODE_TRANSITION
    );
}

void VkRenderBackend::end_frame()
{
    batch2d.reset();
    swap_chain->Present( static_cast<Diligent::Uint32>( vsync_enabled ) );
}

void VkRenderBackend::set_clear_color( Color color )
{
    clear_color[0] = color.r / 255.0f;
    clear_color[1] = color.g / 255.0f;
    clear_color[2] = color.b / 255.0f;
    clear_color[3] = color.a / 255.0f;
}

void VkRenderBackend::set_vsync( bool enabled )
{
    vsync_enabled = enabled;
}

void VkRenderBackend::set_render_size( Vec2 size )
{
    if (swap_chain)
        swap_chain->Resize( static_cast<Diligent::Uint32>( size.X ), static_cast<Diligent::Uint32>( size.Y ) );
}

Vec2 VkRenderBackend::get_surface_size() const
{
    auto& desc = swap_chain->GetDesc();
    return Vec2( static_cast<float>( desc.Width ), static_cast<float>( desc.Height ) );
}

RID VkRenderBackend::create_texture( uint32_t w, uint32_t h, const void* pixels )
{
    RID handle = texture_cache.acquire();

    Diligent::TextureDesc desc;
    desc.Name      = std::move( std::format( "ncore_texture_2d_{}", handle.value ).c_str() );
    desc.Type      = Diligent::RESOURCE_DIM_TEX_2D;
    desc.Width     = static_cast<Diligent::Uint32>( w );
    desc.Height    = static_cast<Diligent::Uint32>( h );
    desc.Format    = Diligent::TEX_FORMAT_RGBA8_UNORM;
    desc.Usage     = Diligent::USAGE_DEFAULT;
    desc.BindFlags = Diligent::BIND_SHADER_RESOURCE;

    Diligent::TextureSubResData mip_0{ pixels, static_cast<Diligent::Uint64>( w * 4 ) };
    Diligent::TextureData init{ &mip_0, 1 };

    DiligentRef<Diligent::ITexture> texture;
    render_device->CreateTexture( desc, &init, &texture );

    auto* entry = texture_cache.get( handle );
    if (entry)
        *entry = std::move( texture );

    return handle;
}

RID VkRenderBackend::create_pipeline( std::string_view vs_spirv, std::string_view ps_spirv )
{
    RID handle = pipeline_cache.acquire();

    DiligentRef<Diligent::IShader> vs;
    {
        Diligent::ShaderCreateInfo ci;
        ci.Desc         = { "user_vs", Diligent::SHADER_TYPE_VERTEX, true };
        ci.ByteCode     = vs_spirv.data();
        ci.ByteCodeSize = vs_spirv.size();
        render_device->CreateShader( ci, &vs );
    }

    DiligentRef<Diligent::IShader> ps;
    {
        Diligent::ShaderCreateInfo ci;
        ci.Desc         = { "user_ps", Diligent::SHADER_TYPE_PIXEL, true };
        ci.ByteCode     = ps_spirv.data();
        ci.ByteCodeSize = ps_spirv.size();
        render_device->CreateShader( ci, &ps );
    }

    DiligentRef<Diligent::IPipelineState> pso;
    {
        Diligent::GraphicsPipelineStateCreateInfo ci;
        ci.PSODesc.Name         = std::move( std::format( "ncore_pso_{}", handle.value ).c_str() );
        ci.PSODesc.PipelineType = Diligent::PIPELINE_TYPE_GRAPHICS;
        ci.pVS                  = vs;
        ci.pPS                  = ps;

        auto& gp                   = ci.GraphicsPipeline;
        gp.NumRenderTargets        = 1;
        gp.RTVFormats[0]           = Diligent::TEX_FORMAT_RGBA8_UNORM_SRGB;
        gp.DSVFormat               = Diligent::TEX_FORMAT_D32_FLOAT;
        gp.PrimitiveTopology       = Diligent::PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
        gp.RasterizerDesc.CullMode = Diligent::CULL_MODE_NONE;

        Diligent::LayoutElement inputs[] = {
            { 0, 0, 2, Diligent::VT_FLOAT32 },
            { 1, 0, 2, Diligent::VT_FLOAT32 },
            { 2, 0, 4, Diligent::VT_UINT8, Diligent::True }
        };
        gp.InputLayout.NumElements    = std::size( inputs );
        gp.InputLayout.LayoutElements = inputs;

        Diligent::ShaderResourceVariableDesc vars[] = {
            { Diligent::SHADER_TYPE_PIXEL, "Texture", Diligent::SHADER_RESOURCE_VARIABLE_TYPE_DYNAMIC }
        };
        ci.PSODesc.ResourceLayout.Variables    = vars;
        ci.PSODesc.ResourceLayout.NumVariables = std::size( vars );

        Diligent::RenderTargetBlendDesc& rt = gp.BlendDesc.RenderTargets[0];
        rt.BlendEnable                      = Diligent::True;
        rt.SrcBlend                         = Diligent::BLEND_FACTOR_SRC_ALPHA;
        rt.DestBlend                        = Diligent::BLEND_FACTOR_INV_SRC_ALPHA;
        rt.BlendOp                          = Diligent::BLEND_OPERATION_ADD;
        rt.RenderTargetWriteMask            = Diligent::COLOR_MASK_ALL;

        render_device->CreateGraphicsPipelineState( ci, &pso );
    }

    auto entry = pipeline_cache.get( handle );
    if (entry) {
        entry->PipelineStateObj = pso;
        pso->CreateShaderResourceBinding( &entry->ShaderResBinding, true );
        entry->TextureVar = entry->ShaderResBinding->GetVariableByName( Diligent::SHADER_TYPE_PIXEL, "Texture" );
    }

    return handle;
}

RID VkRenderBackend::create_buffer( BufferType type, size_t size, const void* data, bool dynamic )
{
    RID handle = buffer_cache.acquire();

    Diligent::BufferDesc desc;
    desc.Size  = static_cast<Diligent::Uint32>( size );
    desc.Usage = Diligent::USAGE_DEFAULT;

    switch (type) {
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

    if (dynamic)
        desc.CPUAccessFlags = Diligent::CPU_ACCESS_WRITE;

    Diligent::BufferData init_data{ data, static_cast<Diligent::Uint32>( size ) };
    DiligentRef<Diligent::IBuffer> buffer;
    render_device->CreateBuffer( desc, data ? &init_data : nullptr, &buffer );

    auto* entry = buffer_cache.get( handle );
    if (entry)
        *entry = std::move( buffer );

    return handle;
}

void VkRenderBackend::destroy_resource( RID rid )
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

void VkRenderBackend::batch_push_quad( RID texture, Vec4 dest, Vec4 src, Color tint )
{
    if (!batch2d || !texture.is_valid())
        return;
    void* native = get_native_texture_view( texture );
    batch2d->push_quad( native ? native : white_tex_view, dest, src, tint );
}

void VkRenderBackend::batch_push_indexed(
    const void* vertices, uint32_t vertex_count, const uint16_t* indices, uint32_t index_count, RID texture,
    Vec4 clip_rect
)
{
    if (!batch2d)
        return;
    void* native = texture.is_valid() ? get_native_texture_view( texture ) : white_tex_view;
    batch2d->push_indexed( vertices, vertex_count, indices, index_count, native, clip_rect );
}

void VkRenderBackend::batch_2d_flush()
{
    if (!batch2d)
        return;

    auto surf_size = get_surface_size();

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

    batch2d->flush( constants_2d, surf_size );
}

void* VkRenderBackend::get_native_texture_view( RID rid ) const
{
    if (auto* t = texture_cache.get( rid )) {
        return ( *t )->GetDefaultView( Diligent::TEXTURE_VIEW_SHADER_RESOURCE );
    }
    return nullptr;
}

void* VkRenderBackend::get_native_device() const
{
    return render_device.RawPtr();
}

void VkRenderBackend::create_2d_pipeline_state_()
{
    DiligentRef<Diligent::IShader> vs_2d;
    {
        Diligent::ShaderCreateInfo shader_ci;
        shader_ci.Desc         = { "2D Vertex Shader", Diligent::SHADER_TYPE_VERTEX, true };
        shader_ci.ByteCode     = GlobalShaders::SPRITE_VERTEX_SPIRV;
        shader_ci.ByteCodeSize = sizeof( GlobalShaders::SPRITE_VERTEX_SPIRV );
        shader_ci.Source       = nullptr;
        render_device->CreateShader( shader_ci, &vs_2d );
    }

    DiligentRef<Diligent::IShader> ps_2d;
    {
        Diligent::ShaderCreateInfo shader_ci;
        shader_ci.Desc  = { "2D Pixel Shader", Diligent::SHADER_TYPE_PIXEL, true };
        const bool srgb = Diligent::GetTextureFormatAttribs( Diligent::TEX_FORMAT_RGBA8_UNORM_SRGB ).ComponentType ==
                          Diligent::COMPONENT_TYPE_UNORM_SRGB;
        if (srgb) {
            shader_ci.ByteCode     = GlobalShaders::SPRITE_FRAGMENT_GAMMA_SPIRV;
            shader_ci.ByteCodeSize = sizeof( GlobalShaders::SPRITE_FRAGMENT_GAMMA_SPIRV );
        } else {
            shader_ci.ByteCode     = GlobalShaders::SPRITE_FRAGMENT_SPIRV;
            shader_ci.ByteCodeSize = sizeof( GlobalShaders::SPRITE_FRAGMENT_SPIRV );
        }
        render_device->CreateShader( shader_ci, &ps_2d );
    }

    Diligent::GraphicsPipelineStateCreateInfo pso_2d_ci;
    pso_2d_ci.PSODesc.Name         = "2D PSO";
    pso_2d_ci.PSODesc.PipelineType = Diligent::PIPELINE_TYPE_GRAPHICS;
    pso_2d_ci.pVS                  = vs_2d;
    pso_2d_ci.pPS                  = ps_2d;

    Diligent::GraphicsPipelineDesc& gp = pso_2d_ci.GraphicsPipeline;
    gp.NumRenderTargets                = 1;
    gp.RTVFormats[0]                   = Diligent::TEX_FORMAT_RGBA8_UNORM_SRGB;
    gp.DSVFormat                       = Diligent::TEX_FORMAT_D32_FLOAT;
    gp.PrimitiveTopology               = Diligent::PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
    gp.RasterizerDesc.CullMode         = Diligent::CULL_MODE_NONE;
    gp.RasterizerDesc.ScissorEnable    = Diligent::True;
    gp.DepthStencilDesc.DepthEnable    = Diligent::False;

    Diligent::RenderTargetBlendDesc& rt = gp.BlendDesc.RenderTargets[0];
    rt.BlendEnable                      = Diligent::True;
    rt.SrcBlend                         = Diligent::BLEND_FACTOR_ONE;
    rt.DestBlend                        = Diligent::BLEND_FACTOR_INV_SRC_ALPHA;
    rt.BlendOp                          = Diligent::BLEND_OPERATION_ADD;
    rt.SrcBlendAlpha                    = Diligent::BLEND_FACTOR_ONE;
    rt.DestBlendAlpha                   = Diligent::BLEND_FACTOR_INV_SRC_ALPHA;
    rt.BlendOpAlpha                     = Diligent::BLEND_OPERATION_ADD;
    rt.RenderTargetWriteMask            = Diligent::COLOR_MASK_ALL;

    Diligent::LayoutElement inputs[] = {
        { 0, 0, 2, Diligent::VT_FLOAT32 },
        { 1, 0, 2, Diligent::VT_FLOAT32 },
        { 2, 0, 4, Diligent::VT_UINT8, Diligent::True }
    };
    gp.InputLayout.NumElements    = std::size( inputs );
    gp.InputLayout.LayoutElements = inputs;

    Diligent::ShaderResourceVariableDesc vars[] = {
        { Diligent::SHADER_TYPE_PIXEL, "Texture", Diligent::SHADER_RESOURCE_VARIABLE_TYPE_DYNAMIC }
    };
    pso_2d_ci.PSODesc.ResourceLayout.Variables    = vars;
    pso_2d_ci.PSODesc.ResourceLayout.NumVariables = std::size( vars );

    Diligent::SamplerDesc sampler;
    sampler.AddressU                                      = Diligent::TEXTURE_ADDRESS_WRAP;
    sampler.AddressV                                      = Diligent::TEXTURE_ADDRESS_WRAP;
    sampler.AddressW                                      = Diligent::TEXTURE_ADDRESS_WRAP;
    Diligent::ImmutableSamplerDesc immut[]                = { { Diligent::SHADER_TYPE_PIXEL, "Texture", sampler } };
    pso_2d_ci.PSODesc.ResourceLayout.ImmutableSamplers    = immut;
    pso_2d_ci.PSODesc.ResourceLayout.NumImmutableSamplers = std::size( immut );

    render_device->CreateGraphicsPipelineState( pso_2d_ci, &pso_2d );

    // Constant buffer for ortho projection
    {
        Diligent::BufferDesc bd;
        bd.Size           = sizeof( Diligent::float4x4 );
        bd.Usage          = Diligent::USAGE_DYNAMIC;
        bd.BindFlags      = Diligent::BIND_UNIFORM_BUFFER;
        bd.CPUAccessFlags = Diligent::CPU_ACCESS_WRITE;
        render_device->CreateBuffer( bd, nullptr, &constants_2d );
    }
    pso_2d->GetStaticVariableByName( Diligent::SHADER_TYPE_VERTEX, "Constants" )->Set( constants_2d );

    pso_2d->CreateShaderResourceBinding( &srb_2d, true );
    texture_var_2d = srb_2d->GetVariableByName( Diligent::SHADER_TYPE_PIXEL, "Texture" );

    // 1x1 white texture
    {
        const uint32_t white = 0xFFFFFFFF;
        Diligent::TextureDesc desc;
        desc.Name      = "2D White Texture";
        desc.Type      = Diligent::RESOURCE_DIM_TEX_2D;
        desc.Width     = 1;
        desc.Height    = 1;
        desc.Format    = Diligent::TEX_FORMAT_RGBA8_UNORM;
        desc.Usage     = Diligent::USAGE_DEFAULT;
        desc.BindFlags = Diligent::BIND_SHADER_RESOURCE;

        Diligent::TextureSubResData mip{ &white, sizeof( white ) };
        Diligent::TextureData init{ &mip, 1 };
        render_device->CreateTexture( desc, &init, &white_texture );
        white_tex_view = white_texture->GetDefaultView( Diligent::TEXTURE_VIEW_SHADER_RESOURCE );
    }
}

} // namespace nc
