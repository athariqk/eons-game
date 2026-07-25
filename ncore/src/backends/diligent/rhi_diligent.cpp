#include "rhi_diligent.h"

#include <BasicTypes.h>
#include <DebugOutput.h>
#include <EngineMemory.h>
#include <GraphicsTypes.h>
#include <MapHelper.hpp>

#include "diligent_allocator.h"
#include "diligent_type_helpers.h"

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

// ---------------------------------------------------------------------------

static NcoreDiligentAllocator allocator;

DiligentRHI::DiligentRHI()
{
    Diligent::SetRawAllocator( &allocator );

    engine_factory = Diligent::LoadAndGetEngineFactoryVk();
    engine_factory->SetMessageCallback( DebugMessageCallbackVk );

    auto vk_version = engine_factory->GetVulkanVersion();
    NC_LOG_INFO_C( log::GRAPHICS, "Vulkan version: {}.{}", vk_version.Major, vk_version.Minor );

    {
        Diligent::EngineVkCreateInfo ci;

#if defined( NC_DEBUG )
        ci.EnableValidation            = true;
        ci.ValidationFlags             = Diligent::VALIDATION_FLAG_CHECK_SHADER_BUFFER_SIZE;
        ci.FeaturesVk.DynamicRendering = Diligent::DEVICE_FEATURE_STATE_OPTIONAL;
#else
        ci.EnableValidation            = false;
        ci.FeaturesVk.DynamicRendering = Diligent::DEVICE_FEATURE_STATE_ENABLED;
#endif

        ci.DynamicHeapSize           = 64 << 20;
        ci.MainDescriptorPoolSize    = { 8192, 1024, 8192, 8192, 1024, 4096, 4096, 1024, 1024, 256, 256 };
        ci.DynamicDescriptorPoolSize = { 2048, 256, 2048, 2048, 256, 1024, 1024, 256, 256, 64, 64 };

        Diligent::ImmediateContextCreateInfo ctxInfo[3] = {
            { "Graphics", 0, Diligent::QUEUE_PRIORITY_MEDIUM },
            { "Compute", 1, Diligent::QUEUE_PRIORITY_MEDIUM },
            { "Transfer", 2, Diligent::QUEUE_PRIORITY_LOW },
        };
        ci.NumImmediateContexts  = 3;
        ci.pImmediateContextInfo = ctxInfo;

        Diligent::IDeviceContext* rawCtx[3] = {};
        engine_factory->CreateDeviceAndContextsVk( ci, &device, rawCtx );
        if (device && rawCtx[0]) {
            ctx_gfx.Attach( rawCtx[0] );
            ctx_gfx->AddRef();
            ctx_comp.Attach( rawCtx[1] );
            ctx_tx.Attach( rawCtx[2] );
        } else {
            NC_LOG_WARN_C( log::GRAPHICS, "Multi-queue init failed, retrying with single context" );
            ci.NumImmediateContexts  = 0;
            ci.pImmediateContextInfo = nullptr;
            engine_factory->CreateDeviceAndContextsVk( ci, &device, &ctx_gfx );
            ctx_comp = ctx_gfx;
            ctx_tx   = ctx_gfx;
        }

        NC_ASSERT( device && ctx_gfx, "Failed to create Vulkan device and contexts" );
    }

    const Diligent::DeviceFeatures& Features = device->GetDeviceInfo().Features;
    if (Features.PipelineStatisticsQueries) {
        Diligent::QueryDesc queryDesc;
        queryDesc.Name = "Pipeline statistics query";
        queryDesc.Type = Diligent::QUERY_TYPE_PIPELINE_STATISTICS;
        pipeline_stats_query.reset( new Diligent::ScopedQueryHelper{ device, queryDesc, 2 } );
    }

    if (Features.OcclusionQueries) {
        Diligent::QueryDesc queryDesc;
        queryDesc.Name = "Occlusion query";
        queryDesc.Type = Diligent::QUERY_TYPE_OCCLUSION;
        occlusion_query.reset( new Diligent::ScopedQueryHelper{ device, queryDesc, 2 } );
    }

    if (Features.TimestampQueries) {
        duration_from_timestamps_query.reset( new Diligent::DurationQueryHelper{ device, 2 } );
    }

    if (Features.DurationQueries) {
        Diligent::QueryDesc queryDesc;
        queryDesc.Name = "Duration query";
        queryDesc.Type = Diligent::QUERY_TYPE_DURATION;
        duration_query.reset( new Diligent::ScopedQueryHelper{ device, queryDesc, 2 } );
    }
}

DiligentRHI::~DiligentRHI()
{
    ctx_gfx->Flush();
    ctx_comp->Flush();
    ctx_tx->Flush();
}

RID DiligentRHI::create_deferred_context( GpuQueue queue )
{
    Diligent::RefCntAutoPtr<Diligent::IDeviceContext> out;
    device->CreateDeferredContext( &out );
    switch (queue) {
        case IRHI::GpuQueue::Graphics: {
            auto rid   = ctx_gfx_defer.acquire();
            auto entry = ctx_gfx_defer.get( rid );
            NC_ASSERT_NULL( entry );
            *entry = out;
            return rid;
        }
        case IRHI::GpuQueue::Compute: {
            auto rid   = ctx_comp_defer.acquire();
            auto entry = ctx_comp_defer.get( rid );
            NC_ASSERT_NULL( entry );
            *entry = out;
            return rid;
        }
        case IRHI::GpuQueue::Transfer: {
            auto rid   = ctx_tx_defer.acquire();
            auto entry = ctx_tx_defer.get( rid );
            NC_ASSERT_NULL( entry );
            *entry = out;
            return rid;
        }
    }
    NC_ASSERT( false, "Unhandled GpuQueue" );
    return 0;
}

void DiligentRHI::set_context_state( bool deferred, RID deferred_id )
{
    is_deferred        = deferred_id.is_valid() ? deferred : false;
    active_deferred_id = deferred_id;
}

void DiligentRHI::set_queue( GpuQueue queue )
{
    active_queue = queue;
}

void DiligentRHI::sync_queue( GpuQueue queue )
{
    auto& ctx = queue == GpuQueue::Graphics ? ctx_gfx : queue == GpuQueue::Compute ? ctx_comp : ctx_tx;
    ctx->WaitForIdle();
}

RID DiligentRHI::swapchain_create( const SwapChainDesc& desc )
{
    if (!desc.native_whnd) {
        NC_LOG_ERROR_C( log::GRAPHICS, "No native window handle supplied for new swap chain" );
        return RID();
    }

    Diligent::NativeWindow native;
#if defined( _WIN32 )
    native.hWnd = desc.native_whnd;
#else
    NC_ASSERT( false, "Native window handle retrieval not implemented for this platform" );
#endif

    Diligent::RefCntAutoPtr<Diligent::ISwapChain> out;
    Diligent::SwapChainDesc ddesc;
    ddesc.ColorBufferFormat = DiligentTypeHelpers::translate_tex_format( desc.color_format );
    ddesc.DepthBufferFormat = DiligentTypeHelpers::translate_tex_format( desc.depth_format );
    ddesc.Usage             = static_cast<Diligent::SWAP_CHAIN_USAGE_FLAGS>( desc.usage );
    ddesc.Width             = static_cast<Diligent::Uint32>( desc.initial_size.X );
    ddesc.Height            = static_cast<Diligent::Uint32>( desc.initial_size.Y );
    ddesc.BufferCount       = static_cast<Diligent::Uint32>( desc.buffer_count );
    ddesc.IsPrimary         = static_cast<Diligent::Bool>( desc.is_primary );
    engine_factory->CreateSwapChainVk( device, ctx_gfx, ddesc, native, &out );
    NC_ASSERT( out, "Failed to create Vulkan swap chain" );

    auto rid    = swapchains.acquire();
    auto handle = swapchains.get( rid );
    NC_ASSERT_NULL( handle );
    *handle = std::move( out );

    return rid;
}

Vec2 DiligentRHI::swapchain_get_size( RID sc )
{
    auto entry = swapchains.get( sc );
    NC_ASSERT_NULL( entry );
    auto width  = static_cast<float>( ( *entry )->GetDesc().Width );
    auto height = static_cast<float>( ( *entry )->GetDesc().Height );
    return Vec2( width, height );
}

void DiligentRHI::swapchain_set_size( RID sc, Vec2 size )
{
    auto entry = swapchains.get( sc );
    NC_ASSERT_NULL( entry );
    auto& desc      = ( *entry )->GetDesc();
    auto new_width  = static_cast<Diligent::Uint32>( size.X );
    auto new_height = static_cast<Diligent::Uint32>( size.Y );
    if (desc.Width != new_width && desc.Height != new_height) {
        ( *entry )->Resize( new_width, new_height );
        NC_LOG_DEBUG_C(
            log::GRAPHICS, "Swapchain RID={} size is different, setting to {}x{}", sc.value, new_width, new_height
        );
    }
}

void DiligentRHI::swapchain_present( RID sc, bool sync_interval )
{
    auto entry = swapchains.get( sc );
    NC_ASSERT_NULL( entry );
    ( *entry )->Present( sync_interval ? 1 : 0 );
}

void* DiligentRHI::swapchain_get_view( RID sc, TextureViewType view )
{
    auto entry = swapchains.get( sc );
    NC_ASSERT_NULL( entry );
    switch (view) {
        case nc::TextureViewType::SHADER_RESOURCE:
            break;
        case nc::TextureViewType::RENDER_TARGET:
            return ( *entry )->GetCurrentBackBufferRTV();
        case nc::TextureViewType::DEPTH_STENCIL:
            return ( *entry )->GetDepthBufferDSV();
        case nc::TextureViewType::UNORDERED_ACCESS:
            break;
    }
    return nullptr;
}

void DiligentRHI::swapchain_destroy( RID target )
{
    NC_LOG_TRACE_C( log::GRAPHICS, "Destroying swap chain (RID: {})", target.value );
    swapchains.release( target );
}

// ---------------------------------------------------------------------------
// Graphics pipeline
// ---------------------------------------------------------------------------

RID DiligentRHI::gfx_pipeline_create( const GraphicsPSODesc& desc, Vector<RID> resource_signatures )
{
    Diligent::RefCntAutoPtr<Diligent::IShader> vs;
    {
        auto name = desc.debug_name + "_VS";
        Diligent::ShaderCreateInfo ci;
        ci.Desc.Name                       = name.c_str();
        ci.Desc.ShaderType                 = Diligent::SHADER_TYPE_VERTEX;
        ci.Desc.UseCombinedTextureSamplers = false;
        ci.LoadConstantBufferReflection    = true;
        ci.ByteCode                        = desc.vs_bytecode.data();
        ci.ByteCodeSize                    = desc.vs_bytecode.size_bytes();
        device->CreateShader( ci, &vs );
    }
    NC_ASSERT_NULL( vs );

    Diligent::RefCntAutoPtr<Diligent::IShader> ps;
    {
        auto name = desc.debug_name + "_PS";
        Diligent::ShaderCreateInfo ci;
        ci.Desc.Name                       = name.c_str();
        ci.Desc.ShaderType                 = Diligent::SHADER_TYPE_PIXEL;
        ci.Desc.UseCombinedTextureSamplers = false;
        ci.LoadConstantBufferReflection    = true;
        ci.ByteCode                        = desc.ps_bytecode.data();
        ci.ByteCodeSize                    = desc.ps_bytecode.size_bytes();
        Diligent::RefCntAutoPtr<Diligent::IDataBlob> blob;
        device->CreateShader( ci, &ps, &blob );
    }
    NC_ASSERT_NULL( ps );

    Diligent::RefCntAutoPtr<Diligent::IPipelineState> pso;
    {
        Diligent::GraphicsPipelineStateCreateInfo ci;
        ci.PSODesc.Name = desc.debug_name.c_str();
        ci.pVS          = vs;
        ci.pPS          = ps;

        auto& gp                                = ci.GraphicsPipeline;
        auto& rsdesc                            = desc.rasterizer_state;
        auto& dsdesc                            = desc.depth_stencil_state;
        auto& bsdesc                            = desc.blend_state.render_targets[0];
        gp.RasterizerDesc.FillMode              = DiligentTypeHelpers::translate_fill_mode( rsdesc.fill );
        gp.RasterizerDesc.CullMode              = DiligentTypeHelpers::translate_cull( rsdesc.cull );
        gp.RasterizerDesc.FrontCounterClockwise = static_cast<Diligent::Bool>( rsdesc.front_ccw );
        gp.RasterizerDesc.DepthBias             = static_cast<Diligent::Int32>( rsdesc.depth_bias_constant );
        gp.RasterizerDesc.SlopeScaledDepthBias  = rsdesc.depth_bias_slope;
        gp.RasterizerDesc.DepthBiasClamp        = rsdesc.depth_bias_clamp;
        gp.RasterizerDesc.DepthClipEnable       = static_cast<Diligent::Bool>( !rsdesc.depth_clamp_enable );
        gp.RasterizerDesc.ScissorEnable         = static_cast<Diligent::Bool>( rsdesc.scissor_enable );
        gp.DepthStencilDesc.DepthEnable         = static_cast<Diligent::Bool>( dsdesc.depth_test );
        gp.DepthStencilDesc.DepthWriteEnable    = static_cast<Diligent::Bool>( dsdesc.depth_write );
        gp.DepthStencilDesc.DepthFunc           = DiligentTypeHelpers::translate_comp_func( dsdesc.depth_func );
        gp.DepthStencilDesc.StencilEnable       = static_cast<Diligent::Bool>( dsdesc.stencil_test );
        gp.DepthStencilDesc.StencilReadMask     = dsdesc.stencil_read_mask;
        gp.DepthStencilDesc.StencilWriteMask    = dsdesc.stencil_write_mask;
        DiligentTypeHelpers::apply_depth_stencil_op( gp.DepthStencilDesc.BackFace, dsdesc.back );
        DiligentTypeHelpers::apply_depth_stencil_op( gp.DepthStencilDesc.FrontFace, dsdesc.front );
        gp.BlendDesc.RenderTargets[0].BlendEnable    = static_cast<Diligent::Bool>( bsdesc.enable );
        gp.BlendDesc.RenderTargets[0].SrcBlend       = DiligentTypeHelpers::translate_blend_factor( bsdesc.src_color );
        gp.BlendDesc.RenderTargets[0].DestBlend      = DiligentTypeHelpers::translate_blend_factor( bsdesc.dst_color );
        gp.BlendDesc.RenderTargets[0].BlendOp        = DiligentTypeHelpers::translate_blend_op( bsdesc.op_color );
        gp.BlendDesc.RenderTargets[0].SrcBlendAlpha  = DiligentTypeHelpers::translate_blend_factor( bsdesc.src_alpha );
        gp.BlendDesc.RenderTargets[0].DestBlendAlpha = DiligentTypeHelpers::translate_blend_factor( bsdesc.dst_alpha );
        gp.BlendDesc.RenderTargets[0].BlendOpAlpha   = DiligentTypeHelpers::translate_blend_op( bsdesc.op_alpha );
        gp.BlendDesc.RenderTargets[0].RenderTargetWriteMask = static_cast<Diligent::COLOR_MASK>( bsdesc.write_mask );
        gp.BlendDesc.AlphaToCoverageEnable  = static_cast<Diligent::Bool>( desc.blend_state.alpha_to_coverage );
        gp.BlendDesc.IndependentBlendEnable = Diligent::False;
        gp.PrimitiveTopology                = DiligentTypeHelpers::translate_prim_topology( desc.primitive_topology );
        gp.NumRenderTargets                 = 1; // TODO: multiple render target
        gp.RTVFormats[0]                    = DiligentTypeHelpers::translate_tex_format( desc.render_target_format );
        gp.DSVFormat                        = DiligentTypeHelpers::translate_tex_format( desc.depth_stencil_format );
        gp.NumViewports                     = 1; // TODO: multiple viewports
        gp.SmplDesc.Count                   = desc.multisample_state.count;
        gp.SmplDesc.Quality                 = desc.multisample_state.quality;

        Vector<Diligent::LayoutElement> inputs;
        for (const auto& elem : desc.vert_layout) {
            Diligent::LayoutElement le{};
            le.HLSLSemantic         = elem.hlsl_semantic;
            le.InputIndex           = elem.location;
            le.BufferSlot           = elem.buffer_slot;
            le.NumComponents        = DiligentTypeHelpers::translate_value_num_components( elem.type );
            le.ValueType            = DiligentTypeHelpers::translate_value_type( elem.type );
            le.IsNormalized         = static_cast<Diligent::Bool>( elem.normalized );
            le.RelativeOffset       = elem.relative_offset;
            le.Stride               = static_cast<Diligent::Uint32>( elem.stride );
            le.Frequency            = DiligentTypeHelpers::translate_vertex_frequency( elem.frequency );
            le.InstanceDataStepRate = elem.instance_step_rate;
            inputs.push_back( le );
        }
        ci.GraphicsPipeline.InputLayout.NumElements    = static_cast<Diligent::Uint32>( inputs.size() );
        ci.GraphicsPipeline.InputLayout.LayoutElements = inputs.data();

        Vector<Diligent::IPipelineResourceSignature*> sigs;
        for (auto& rid : resource_signatures) {
            auto entry = res_signatures.get( rid );
            NC_ASSERT( entry->RawPtr(), "A valid resource signature is required for PSO creation" );
            sigs.push_back( entry->RawPtr() );
        }
        if (!sigs.empty()) {
            ci.ppResourceSignatures    = sigs.data();
            ci.ResourceSignaturesCount = static_cast<Diligent::Uint32>( sigs.size() );
        }

        device->CreateGraphicsPipelineState( ci, &pso );

        if (pso) {
            std::string layout_summary;
            for (auto& e : inputs) {
                if (!layout_summary.empty())
                    layout_summary += ", ";
                layout_summary += std::format( "slot{}:loc{}", e.BufferSlot, e.InputIndex );
            }
            NC_LOG_INFO_C(
                log::GRAPHICS, "PSO '{}' created OK. Layout=[{}] sigs={} RTV={:#x}", desc.debug_name, layout_summary,
                resource_signatures.size(), static_cast<int>( desc.render_target_format )
            );
        } else {
            NC_LOG_ERROR_C( log::GRAPHICS, "FAILED to create PSO '{}'", desc.debug_name );
        }
    }
    NC_ASSERT_NULL( pso );

    RID handle = pipelines.acquire();
    auto entry = pipelines.get( handle );
    NC_ASSERT( entry, "Failed to acquire new pipeline state object" );
    *entry = std::move( pso );

    return handle;
}

void DiligentRHI::gfx_pipeline_bind( RID pipeline )
{
    auto entry = pipelines.get( pipeline );
    NC_ASSERT( entry, "Invalid pipeline" );
    NC_LOG_TRACE_C( log::GRAPHICS, "gfx_pipeline_bind: rid={}", pipeline.value );
    get_active_ctx_()->SetPipelineState( *entry );
}

void DiligentRHI::gfx_pipeline_reload( RID pipeline )
{
    auto entry = pipelines.get( pipeline );
    NC_ASSERT( entry, "Invalid pipeline" );
    entry->Release();
    pipelines.release( pipeline );
}

// ---------------------------------------------------------------------------

void DiligentRHI::render_target_bind( std::span<const void*> rtvs, void* dsv )
{
    auto views    = reinterpret_cast<Diligent::ITextureView**>( const_cast<void**>( rtvs.data() ) );
    auto dsv_view = static_cast<Diligent::ITextureView*>( dsv );
    get_active_ctx_()->SetRenderTargets(
        static_cast<Diligent::Uint32>( rtvs.size() ), views, dsv_view,
        Diligent::RESOURCE_STATE_TRANSITION_MODE_TRANSITION
    );
}

void DiligentRHI::render_target_set_viewport( std::span<const Viewport> p_viewports )
{
    Vector<Diligent::Viewport> vps;
    for (auto& vp : p_viewports) {
        vps.push_back( { vp.rect.X, vp.rect.Y, vp.rect.W, vp.rect.H, vp.min_depth, vp.max_depth } );
    }
    NC_LOG_TRACE_C(
        log::GRAPHICS, "set_viewport: {} viewports (first: {:.0f},{:.0f} {:.0f}x{:.0f})", vps.size(), vps[0].TopLeftX,
        vps[0].TopLeftY, vps[0].Width, vps[0].Height
    );
    get_active_ctx_()->SetViewports( static_cast<Diligent::Uint32>( vps.size() ), vps.data(), 0, 0 );
}

void DiligentRHI::render_target_set_scissor_rect( std::span<const Vec4> p_rects )
{
    Vector<Diligent::Rect> rects;
    for (auto r : p_rects) {
        rects.push_back(
            Diligent::Rect{
                static_cast<Diligent::Int32>( r.X ), static_cast<Diligent::Int32>( r.Y ),
                static_cast<Diligent::Int32>( r.X + r.W ), static_cast<Diligent::Int32>( r.Y + r.H )
            }
        );
    }
    NC_LOG_TRACE_C(
        log::GRAPHICS, "set_scissor: {} rects (first: {},{} {},{})", rects.size(), rects[0].left, rects[0].top,
        rects[0].right, rects[0].bottom
    );
    get_active_ctx_()->SetScissorRects( static_cast<Diligent::Uint32>( p_rects.size() ), rects.data(), 0, 0 );
}

void DiligentRHI::render_target_clear_color( void* rtv, const Color& color )
{
    auto* view          = static_cast<Diligent::ITextureView*>( rtv );
    const float rgba[4] = { color.r / 255.0f, color.g / 255.0f, color.b / 255.0f, color.a / 255.0f };
    NC_LOG_TRACE_C( log::GRAPHICS, "clear_color: r={} g={} b={} a={}", color.r, color.g, color.b, color.a );
    get_active_ctx_()->ClearRenderTarget( view, rgba, Diligent::RESOURCE_STATE_TRANSITION_MODE_TRANSITION );
}

void DiligentRHI::render_target_clear_depth( void* dsv, float depth, uint8_t stencil )
{
    auto* view = static_cast<Diligent::ITextureView*>( dsv );
    get_active_ctx_()->ClearDepthStencil(
        view, Diligent::CLEAR_DEPTH_FLAG | Diligent::CLEAR_STENCIL_FLAG, depth, stencil,
        Diligent::RESOURCE_STATE_TRANSITION_MODE_TRANSITION
    );
}

void DiligentRHI::commands_record_begin()
{
    get_active_ctx_()->Begin( get_imm_ctx_()->GetDesc().ContextId );
}

void* DiligentRHI::commands_record_end()
{
    get_active_ctx_()->FinishCommandList( cmd_list.ReleaseAndGetAddressOf() );
    return cmd_list.RawPtr();
}

void DiligentRHI::commands_record_execute( void* p_cmd_list )
{
    auto casted                     = reinterpret_cast<Diligent::ICommandList*>( p_cmd_list );
    Diligent::ICommandList* temp[1] = { casted };
    get_active_ctx_()->ExecuteCommandLists( 1, temp );
}

void DiligentRHI::commands_release()
{
    get_active_ctx_()->FinishFrame();
}

// ---------------------------------------------------------------------------

void DiligentRHI::compute_pipeline_create() {}

// ---------------------------------------------------------------------------
// Textures
// ---------------------------------------------------------------------------

RID DiligentRHI::texture_create( const TextureDesc& desc )
{
    Diligent::TextureDesc ddesc;
    ddesc.Name                 = desc.debug_name.c_str();
    ddesc.Type                 = DiligentTypeHelpers::translate_resource_dim( desc.dimension );
    ddesc.Width                = desc.width;
    ddesc.Height               = desc.height;
    ddesc.Format               = DiligentTypeHelpers::translate_tex_format( desc.format );
    ddesc.ArraySize            = desc.array_size;
    ddesc.MipLevels            = desc.mip_levels;
    ddesc.SampleCount          = desc.sample_count;
    ddesc.Usage                = static_cast<Diligent::USAGE>( desc.usage );
    ddesc.BindFlags            = static_cast<Diligent::BIND_FLAGS>( desc.bind_mask );
    ddesc.CPUAccessFlags       = static_cast<Diligent::CPU_ACCESS_FLAGS>( desc.access_mask );
    ddesc.MiscFlags            = Diligent::MISC_TEXTURE_FLAG_NONE;
    ddesc.ImmediateContextMask = 1;

    Diligent::TextureSubResData mip_0{ desc.pixels, static_cast<Diligent::Uint64>( desc.width * 4 ) };
    Diligent::TextureData init{ &mip_0, 1 };

    Diligent::RefCntAutoPtr<Diligent::ITexture> texture;
    device->CreateTexture( ddesc, &init, &texture );

    RID handle = textures.acquire();
    auto entry = textures.get( handle );
    NC_ASSERT( entry, "Failed acquiring new texture slot" );
    *entry = std::move( texture );

    return handle;
}

void DiligentRHI::texture_binding_update( RID texture, RID binding, const char* name )
{
    auto srb_entry = res_bindings.get( binding );
    NC_ASSERT( srb_entry, "Invalid SRB" );

    auto tex_entry = textures.get( texture );
    NC_ASSERT( tex_entry, "Invalid texture" );

    auto view = ( *tex_entry )->GetDefaultView( Diligent::TEXTURE_VIEW_SHADER_RESOURCE );
    NC_ASSERT_NULL( view );

    auto var = ( *srb_entry )->GetVariableByName( Diligent::SHADER_TYPE_PIXEL, name );
    if (!var)
        NC_LOG_ERROR_C( log::GRAPHICS, "texture_binding_update: var '{}' NOT FOUND in PIXEL stage", name );
    NC_ASSERT_NULL( var );

    var->Set( view, Diligent::SET_SHADER_RESOURCE_FLAG_ALLOW_OVERWRITE );
}

void* DiligentRHI::texture_get_view( RID texture, TextureViewType view )
{
    auto entry = textures.get( texture );
    NC_ASSERT( entry, "Invalid texture" );

    auto map_view = []( TextureViewType t ) {
        switch (t) {
            case TextureViewType::SHADER_RESOURCE:
                return Diligent::TEXTURE_VIEW_SHADER_RESOURCE;
            case TextureViewType::RENDER_TARGET:
                return Diligent::TEXTURE_VIEW_RENDER_TARGET;
            case TextureViewType::DEPTH_STENCIL:
                return Diligent::TEXTURE_VIEW_DEPTH_STENCIL;
            case TextureViewType::UNORDERED_ACCESS:
                return Diligent::TEXTURE_VIEW_UNORDERED_ACCESS;
        }
        return Diligent::TEXTURE_VIEW_SHADER_RESOURCE;
    };
    return ( *entry )->GetDefaultView( map_view( view ) );
}

RID DiligentRHI::sampler_create( const SamplerDesc& desc )
{
    Diligent::SamplerDesc sd;
    sd.MinFilter = desc.min_filter == SamplerFilter::NEAREST
                       ? Diligent::FILTER_TYPE_POINT
                       : ( desc.min_filter == SamplerFilter::ANISOTROPIC ? Diligent::FILTER_TYPE_ANISOTROPIC
                                                                         : Diligent::FILTER_TYPE_LINEAR );
    sd.MagFilter = desc.mag_filter == SamplerFilter::NEAREST
                       ? Diligent::FILTER_TYPE_POINT
                       : ( desc.mag_filter == SamplerFilter::ANISOTROPIC ? Diligent::FILTER_TYPE_ANISOTROPIC
                                                                         : Diligent::FILTER_TYPE_LINEAR );
    sd.MipFilter = desc.mip_filter == SamplerFilter::NEAREST
                       ? Diligent::FILTER_TYPE_POINT
                       : ( desc.mip_filter == SamplerFilter::ANISOTROPIC ? Diligent::FILTER_TYPE_ANISOTROPIC
                                                                         : Diligent::FILTER_TYPE_LINEAR );

    auto translate_address = []( TextureAddressMode m ) {
        switch (m) {
            case TextureAddressMode::WRAP:
                return Diligent::TEXTURE_ADDRESS_WRAP;
            case TextureAddressMode::MIRROR:
                return Diligent::TEXTURE_ADDRESS_MIRROR;
            case TextureAddressMode::BORDER:
                return Diligent::TEXTURE_ADDRESS_BORDER;
            case TextureAddressMode::CLAMP:
                return Diligent::TEXTURE_ADDRESS_CLAMP;
        }
    };
    sd.AddressU = translate_address( desc.address_u );
    sd.AddressV = translate_address( desc.address_v );
    sd.AddressW = translate_address( desc.address_w );

    Diligent::RefCntAutoPtr<Diligent::ISampler> sampler;
    device->CreateSampler( sd, &sampler );

    RID handle = samplers.acquire();
    auto entry = samplers.get( handle );
    NC_ASSERT( entry, "Failed acquiring new sampler slot" );
    *entry = std::move( sampler );

    return handle;
}

void DiligentRHI::sampler_update_binding( RID sampler, RID binding, const char* name )
{
    auto* srb_entry = res_bindings.get( binding );
    NC_ASSERT( srb_entry, "Invalid SRB" );

    auto* sam_entry = samplers.get( sampler );
    NC_ASSERT( sam_entry, "Invalid sampler" );

    auto* var = ( *srb_entry )->GetVariableByName( Diligent::SHADER_TYPE_PIXEL, name );
    if (!var)
        NC_LOG_ERROR_C( log::GRAPHICS, "sampler_update_binding: var '{}' NOT FOUND in PIXEL stage", name );
    NC_ASSERT_NULL( var );

    var->Set( *sam_entry );
    NC_LOG_TRACE_C( log::GRAPHICS, "sampler_update_binding: var='{}' bound to sampler rid={}", name, sampler.value );
}

// ---------------------------------------------------------------------------
// Buffers
// ---------------------------------------------------------------------------

RID DiligentRHI::buffer_create( const BufferDesc& p_desc )
{
    Diligent::BufferDesc desc;
    desc.Name           = p_desc.debug_name.data();
    desc.Size           = static_cast<Diligent::Uint32>( p_desc.size );
    desc.BindFlags      = static_cast<Diligent::BIND_FLAGS>( p_desc.bind_mask );
    desc.Usage          = static_cast<Diligent::USAGE>( p_desc.usage );
    desc.CPUAccessFlags = static_cast<Diligent::CPU_ACCESS_FLAGS>( p_desc.access_mask );

    Diligent::BufferData init_data{ p_desc.initial_data, static_cast<Diligent::Uint32>( p_desc.size ) };
    Diligent::RefCntAutoPtr<Diligent::IBuffer> buffer;
    device->CreateBuffer( desc, p_desc.initial_data ? &init_data : nullptr, &buffer );

    RID handle = buffers.acquire();
    auto entry = buffers.get( handle );
    NC_ASSERT( entry, "Failed acquiring new buffer slot" );
    *entry = std::move( buffer );

    NC_LOG_DEBUG_C(
        log::GRAPHICS, "buffer_create: name='{}' size={} usage={} bind={:#x} cpu={:#x} rid={}", p_desc.debug_name,
        p_desc.size, static_cast<int>( p_desc.usage ), static_cast<int>( p_desc.bind_mask ),
        static_cast<int>( p_desc.access_mask ), handle.value
    );

    return handle;
}

void DiligentRHI::buffer_update( RID buffer, const void* data, size_t size )
{
    auto buf = buffers.get( buffer );
    NC_ASSERT( buf, "Invalid buffer" );

    Diligent::MapHelper<uint8_t> map{ get_active_ctx_(), *buf, Diligent::MAP_WRITE, Diligent::MAP_FLAG_DISCARD };
    if (map) {
        memcpy( map, data, size );
        NC_LOG_TRACE_C( log::GRAPHICS, "buffer_update: rid={} size={} OK", buffer.value, size );
    } else {
        NC_LOG_ERROR_C( log::GRAPHICS, "buffer_update: rid={} size={} MAP FAILED", buffer.value, size );
    }
}

void DiligentRHI::buffer_update_binding( RID buffer, RID binding, const char* name )
{
    auto srb_entry = res_bindings.get( binding );
    NC_ASSERT( srb_entry, "Invalid SRB" );

    auto buf_entry = buffers.get( buffer );
    NC_ASSERT( buf_entry, "Invalid buffer" );

    auto var = ( *srb_entry )->GetVariableByName( Diligent::SHADER_TYPE_VERTEX, name );
    if (!var)
        NC_LOG_ERROR_C( log::GRAPHICS, "buffer_update_binding: var '{}' NOT FOUND in VERTEX stage", name );
    NC_ASSERT_NULL( var );

    var->Set( *buf_entry );
    NC_LOG_TRACE_C( log::GRAPHICS, "buffer_update_binding: var='{}' bound to buffer rid={}", name, buffer.value );
}

void DiligentRHI::vertex_buffers_bind(
    std::span<const RID> p_buffers, uint32_t slot, std::span<const uint64_t> offsets
)
{
    Vector<Diligent::IBuffer*> buffer_ptrs;
    for (auto& rid : p_buffers) {
        auto ptr = buffers.get( rid );
        NC_ASSERT_NULL( ptr );
        buffer_ptrs.push_back( ptr->RawPtr() );
    }

    NC_LOG_TRACE_C( log::GRAPHICS, "vertex_buffers_bind: {} buffers at slot={}", p_buffers.size(), slot );
    get_active_ctx_()->SetVertexBuffers(
        slot, static_cast<Diligent::Uint32>( buffer_ptrs.size() ), buffer_ptrs.data(), offsets.data(),
        Diligent::RESOURCE_STATE_TRANSITION_MODE_TRANSITION, Diligent::SET_VERTEX_BUFFERS_FLAG_RESET
    );
}

void DiligentRHI::index_buffer_bind( RID buffer, uint32_t offset )
{
    auto buf = buffers.get( buffer );
    NC_ASSERT_NULL( buf );
    NC_LOG_TRACE_C( log::GRAPHICS, "index_buffer_bind: rid={} offset={}", buffer.value, offset );
    get_active_ctx_()->SetIndexBuffer( *buf, offset, Diligent::RESOURCE_STATE_TRANSITION_MODE_TRANSITION );
}

// ---------------------------------------------------------------------------

RID DiligentRHI::resource_signature_create( const ResourceSignatureDesc& desc )
{
    Diligent::PipelineResourceSignatureDesc pdesc{};
    pdesc.BindingIndex = desc.set;
    pdesc.Name         = desc.name.c_str();

    Vector<Diligent::PipelineResourceDesc> rdescs;
    for (const auto& rd : desc.resources) {
        Diligent::PipelineResourceDesc rdesc{};
        rdesc.Name         = rd.name.c_str();
        rdesc.ResourceType = DiligentTypeHelpers::translate_resource_type( rd.resource_type );
        rdesc.ArraySize    = rd.array_size;
        rdesc.Flags        = DiligentTypeHelpers::translate_pipeline_resource_flags( rd.flags );
        rdesc.ShaderStages = DiligentTypeHelpers::translate_shader_stage( rd.stage );
        rdesc.VarType      = DiligentTypeHelpers::translate_shader_resource_var_type( rd.var_type );
        rdescs.push_back( std::move( rdesc ) );
    }

    pdesc.Resources    = rdescs.data();
    pdesc.NumResources = static_cast<Diligent::Uint32>( rdescs.size() );

    Diligent::RefCntAutoPtr<Diligent::IPipelineResourceSignature> sig;
    device->CreatePipelineResourceSignature( pdesc, &sig );
    if (!sig) {
        NC_LOG_ERROR_C( log::GRAPHICS, "FAILED to create resource signature '{}' (set {})", desc.name, desc.set );
    } else {
        NC_LOG_DEBUG_C(
            log::GRAPHICS, "Resource signature '{}' (set {}) created with {} resources", desc.name, desc.set,
            rdescs.size()
        );
        for (auto& r : rdescs) {
            NC_LOG_DEBUG_C(
                log::GRAPHICS, "  sig resource: '{}' type={} stage={}", r.Name, static_cast<int>( r.ResourceType ),
                static_cast<int>( r.ShaderStages )
            );
        }
    }
    NC_ASSERT(
        sig.RawPtr(), std::format( "Failed to create resource signature '{}' (set {})", desc.name, desc.set ).c_str()
    );

    RID handle = res_signatures.acquire();
    auto entry = res_signatures.get( handle );
    NC_ASSERT( entry, "Failed acquiring new resource signature slot" );
    *entry = std::move( sig );

    return handle;
}

RID DiligentRHI::resource_binding_create( RID signature )
{
    auto sig = res_signatures.get( signature );
    NC_ASSERT( sig, "Invalid resource signature" );

    Diligent::RefCntAutoPtr<Diligent::IShaderResourceBinding> srb;
    ( *sig )->CreateShaderResourceBinding( &srb, true );

    RID handle = res_bindings.acquire();
    auto entry = res_bindings.get( handle );
    NC_ASSERT( entry, "Failed acquiring new resource binding slot" );
    *entry = std::move( srb );

    return handle;
}

void DiligentRHI::resource_binding_commit( RID binding )
{
    auto srb = res_bindings.get( binding );
    NC_ASSERT( srb, "Invalid SRB" );
    NC_LOG_TRACE_C( log::GRAPHICS, "resource_binding_commit: rid={}", binding.value );
    get_active_ctx_()->CommitShaderResources( *srb, Diligent::RESOURCE_STATE_TRANSITION_MODE_TRANSITION );
}

// ---------------------------------------------------------------------------

bool DiligentRHI::is_rid_owned( RID rid )
{
    if (textures.contains( rid ))
        return true;
    if (pipelines.contains( rid ))
        return true;
    if (buffers.contains( rid ))
        return true;
    if (samplers.contains( rid ))
        return true;
    if (res_signatures.contains( rid ))
        return true;
    if (res_bindings.get( rid ))
        return true;
    return false;
}

void DiligentRHI::destroy_resource( RID rid )
{
    if (textures.get( rid )) {
        textures.release( rid );
        return;
    }
    if (pipelines.get( rid )) {
        pipelines.release( rid );
        return;
    }
    if (buffers.get( rid )) {
        buffers.release( rid );
        return;
    }
    if (samplers.get( rid )) {
        samplers.release( rid );
        return;
    }
    if (res_signatures.get( rid )) {
        res_signatures.release( rid );
        return;
    }
    if (res_bindings.get( rid )) {
        res_bindings.release( rid );
        return;
    }
}

// ---------------------------------------------------------------------------
// Draw
// ---------------------------------------------------------------------------

void DiligentRHI::draw( uint32_t vertex_count, uint32_t start_vertex, uint32_t instance_count )
{
    NC_LOG_TRACE_C( log::GRAPHICS, "draw: verts={} start={} instances={}", vertex_count, start_vertex, instance_count );
    Diligent::DrawAttribs attrs;
    attrs.NumVertices         = vertex_count;
    attrs.StartVertexLocation = start_vertex;
    attrs.NumInstances        = instance_count;
    attrs.Flags               = Diligent::DRAW_FLAG_VERIFY_ALL;
    get_active_ctx_()->Draw( attrs );
}

void DiligentRHI::draw_indexed(
    uint32_t index_count, uint32_t start_index, int32_t base_vertex, uint32_t instance_count
)
{
    NC_LOG_TRACE_C(
        log::GRAPHICS, "draw_indexed: indices={} start={} baseVtx={} instances={}", index_count, start_index,
        base_vertex, instance_count
    );
    Diligent::DrawIndexedAttribs attrs;
    attrs.NumIndices         = index_count;
    attrs.FirstIndexLocation = start_index;
    attrs.BaseVertex         = base_vertex;
    attrs.NumInstances       = instance_count;
    attrs.IndexType          = Diligent::VT_UINT16;
    attrs.Flags              = Diligent::DRAW_FLAG_VERIFY_ALL;
    get_active_ctx_()->DrawIndexed( attrs );
}

// ---------------------------------------------------------------------------

void DiligentRHI::load_pso_cache() {}
void DiligentRHI::save_pso_cache() {}

void DiligentRHI::begin_queries()
{
    if (pipeline_stats_query)
        pipeline_stats_query->Begin( get_active_ctx_() );
    if (occlusion_query)
        occlusion_query->Begin( get_active_ctx_() );
    if (duration_from_timestamps_query)
        duration_from_timestamps_query->Begin( get_active_ctx_() );
    if (duration_query)
        duration_query->Begin( get_active_ctx_() );
}

void DiligentRHI::end_queries()
{
    if (duration_from_timestamps_query)
        duration_from_timestamps_query->End( get_active_ctx_(), duration_from_timestamps );
    if (duration_query)
        duration_query->End( get_active_ctx_(), &duration_data, sizeof( duration_data ) );
    if (occlusion_query)
        occlusion_query->End( get_active_ctx_(), &occlusion_data, sizeof( occlusion_data ) );
    if (pipeline_stats_query)
        pipeline_stats_query->End( get_active_ctx_(), &pipeline_stats_data, sizeof( pipeline_stats_data ) );
}

IRHI::Stats DiligentRHI::get_stats()
{
    Stats stats;

    if (pipeline_stats_query) {
        stats.input_vertices       = pipeline_stats_data.InputVertices;
        stats.input_primitives     = pipeline_stats_data.InputPrimitives;
        stats.vs_invocations       = pipeline_stats_data.VSInvocations;
        stats.gs_invocations       = pipeline_stats_data.GSInvocations;
        stats.ps_invocations       = pipeline_stats_data.PSInvocations;
        stats.clipping_invocations = pipeline_stats_data.ClippingInvocations;
        stats.clipping_primitives  = pipeline_stats_data.ClippingPrimitives;
    }

    if (occlusion_query)
        stats.occlusion_samples_passed = occlusion_data.NumSamples;

    if (duration_from_timestamps_query)
        stats.gpu_duration_ms = duration_from_timestamps * 1000.0;

    return stats;
}

// ---------------------------------------------------------------------------

Diligent::IDeviceContext* DiligentRHI::get_active_ctx_()
{
    auto resolve = [this]( auto& defer_map, auto& immediate_ctx, const char* err_msg ) -> Diligent::IDeviceContext* {
        if (is_deferred) {
            auto ref = defer_map.get( active_deferred_id );
            NC_ASSERT_NULL_MSG( ref, err_msg );
            return ref->RawPtr();
        }
        NC_ASSERT( immediate_ctx, err_msg );
        return immediate_ctx.RawPtr();
    };

    switch (active_queue) {
        case GpuQueue::Graphics:
            return resolve( ctx_gfx_defer, ctx_gfx, "Invalid gfx context ref" );
        case GpuQueue::Compute:
            return resolve( ctx_comp_defer, ctx_comp, "Invalid compute context ref" );
        case GpuQueue::Transfer:
            return resolve( ctx_tx_defer, ctx_tx, "Invalid transfer context ref" );
    }
    return nullptr;
}

Diligent::IDeviceContext* DiligentRHI::get_imm_ctx_()
{
    switch (active_queue) {
        case GpuQueue::Graphics:
            return ctx_gfx.RawPtr();
        case GpuQueue::Compute:
            return ctx_comp.RawPtr();
        case GpuQueue::Transfer:
            return ctx_tx.RawPtr();
    }
    return nullptr;
}

} // namespace nc
