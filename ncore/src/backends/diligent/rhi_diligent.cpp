#include "rhi_diligent.h"

#include <BasicTypes.h>
#include <DebugOutput.h>
#include <EngineMemory.h>
#include <GraphicsTypes.h>
#include <MapHelper.hpp>

#include "diligent_allocator.h"
#include "diligent_type_helpers.h"

#include <vulkan/vulkan_core.h>

namespace nc {

static void DILIGENT_CALL_TYPE OutputMessageCallbackDiligent(
    enum Diligent::DEBUG_MESSAGE_SEVERITY Severity, const Diligent::Char* Message, const Diligent::Char* Function,
    const Diligent::Char* File, int Line
)
{
    switch (Severity) {
        case Diligent::DEBUG_MESSAGE_SEVERITY_INFO:
            NC_LOG( log::GRAPHICS, log::Level::LINFO, File, Function, Line, "{}", Message );
            break;
        case Diligent::DEBUG_MESSAGE_SEVERITY_WARNING:
            NC_LOG( log::GRAPHICS, log::Level::LWARN, File, Function, Line, "{}", Message );
            break;
        case Diligent::DEBUG_MESSAGE_SEVERITY_ERROR:
            NC_LOG( log::GRAPHICS, log::Level::LERROR, File, Function, Line, "{}", Message );
            break;
        case Diligent::DEBUG_MESSAGE_SEVERITY_FATAL_ERROR:
            NC_LOG( log::GRAPHICS, log::Level::LFATAL, File, Function, Line, "{}", Message );
            break;
    }
}

// ---------------------------------------------------------------------------

static NcoreDiligentAllocator allocator;

DiligentRHI::DiligentRHI()
{
    Diligent::SetRawAllocator( &allocator );

    engine_factory = Diligent::LoadAndGetEngineFactoryVk();
    engine_factory->SetMessageCallback( OutputMessageCallbackDiligent );

    auto vk_version = engine_factory->GetVulkanVersion();
    NC_LOG_INFO_C( log::GRAPHICS, "Vulkan version: {}.{}", vk_version.Major, vk_version.Minor );

    Diligent::Uint32 num_adapters;
    engine_factory->EnumerateAdapters( vk_version, num_adapters, nullptr );
    NC_ASSERT( num_adapters > 0, "This machine has no video adapter, this is catastrophic" );
    DynamicArray<Diligent::GraphicsAdapterInfo> adapters;
    adapters.resize( num_adapters );
    engine_factory->EnumerateAdapters( vk_version, num_adapters, adapters.data() );

    {
        Diligent::EngineVkCreateInfo ci;

        // Required for SPIR-V DrawParameters capability emitted by Slang when
        // shaders use SV_InstanceID / SV_VertexID.
        VkPhysicalDeviceVulkan11Features vk11_features{};
        vk11_features.sType                = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_1_FEATURES;
        vk11_features.shaderDrawParameters = VK_TRUE;
        ci.pDeviceExtensionFeatures        = &vk11_features;

#if defined( NC_DEBUG )
        ci.EnableValidation            = true;
        ci.ValidationFlags             = Diligent::VALIDATION_FLAG_CHECK_SHADER_BUFFER_SIZE;
        ci.FeaturesVk.DynamicRendering = Diligent::DEVICE_FEATURE_STATE_OPTIONAL;
#else
        ci.EnableValidation            = false;
        ci.FeaturesVk.DynamicRendering = Diligent::DEVICE_FEATURE_STATE_ENABLED;
#endif
        ci.DynamicHeapSize                    = 64 << 20;
        ci.MainDescriptorPoolSize             = { 8192, 1024, 8192, 8192, 1024, 4096, 4096, 1024, 1024, 256, 256 };
        ci.DynamicDescriptorPoolSize          = { 2048, 256, 2048, 2048, 256, 1024, 1024, 256, 256, 64, 64 };
        ci.Features.MultiViewport             = Diligent::DEVICE_FEATURE_STATE_OPTIONAL;
        ci.Features.PipelineStatisticsQueries = Diligent::DEVICE_FEATURE_STATE_OPTIONAL;
        ci.Features.OcclusionQueries          = Diligent::DEVICE_FEATURE_STATE_OPTIONAL;
        ci.Features.TimestampQueries          = Diligent::DEVICE_FEATURE_STATE_OPTIONAL;
        ci.Features.WireframeFill             = Diligent::DEVICE_FEATURE_STATE_OPTIONAL;

        // defaulting to the zeroth adapter (might be a integrated gfx on mobile platforms)
        Diligent::Uint32 adapter_index = 0;
        ci.AdapterId                   = adapter_index;
        auto& adapter_info             = adapters[adapter_index];
        for (adapter_index = 0; adapter_index < num_adapters; ++adapter_index) {
            // search through all adapters till we hit discrete gfx
            if (adapters[adapter_index].Type == Diligent::ADAPTER_TYPE_DISCRETE) {
                ci.AdapterId = adapter_index;
                adapter_info = adapters[adapter_index];
                break;
            }
        }

        // find correct QueueId indices based on actual QueueType bitmasks
        Diligent::Uint8 gfx_queue_id  = Diligent::COMMAND_QUEUE_TYPE_UNKNOWN;
        Diligent::Uint8 comp_queue_id = Diligent::COMMAND_QUEUE_TYPE_UNKNOWN;
        Diligent::Uint8 tx_queue_id   = Diligent::COMMAND_QUEUE_TYPE_UNKNOWN;

        for (Diligent::Uint32 q = 0; q < adapter_info.NumQueues; ++q) {
            const auto& q_info = adapter_info.Queues[q];

            // pick primary queue with Graphics support
            if (gfx_queue_id == Diligent::COMMAND_QUEUE_TYPE_UNKNOWN &&
                ( q_info.QueueType & Diligent::COMMAND_QUEUE_TYPE_GRAPHICS )) {
                gfx_queue_id = static_cast<Diligent::Uint8>( q );
            }
            // pick queue with Compute support (prefer queue 2 if available for Async Compute)
            if (( q_info.QueueType & Diligent::COMMAND_QUEUE_TYPE_COMPUTE ) &&
                ( comp_queue_id == Diligent::COMMAND_QUEUE_TYPE_UNKNOWN || q == 2 )) {
                comp_queue_id = static_cast<Diligent::Uint8>( q );
            }
            // pick queue with Transfer support
            if (tx_queue_id == Diligent::COMMAND_QUEUE_TYPE_UNKNOWN &&
                ( q_info.QueueType & Diligent::COMMAND_QUEUE_TYPE_TRANSFER )) {
                tx_queue_id = static_cast<Diligent::Uint8>( q );
            }
        }

        // fallback to Queue 0 if dedicated capabilities weren't found on isolated queues
        if (gfx_queue_id == Diligent::COMMAND_QUEUE_TYPE_UNKNOWN)
            gfx_queue_id = 0;
        if (comp_queue_id == Diligent::COMMAND_QUEUE_TYPE_UNKNOWN)
            comp_queue_id = gfx_queue_id;
        if (tx_queue_id == Diligent::COMMAND_QUEUE_TYPE_UNKNOWN)
            tx_queue_id = gfx_queue_id;

        Diligent::ImmediateContextCreateInfo ctx_info[3] = {
            { "Graphics", gfx_queue_id, Diligent::QUEUE_PRIORITY_MEDIUM },
            { "Compute", comp_queue_id, Diligent::QUEUE_PRIORITY_MEDIUM },
            // TODO: diligent won't let us have different queue priorities for some reason
            { "Transfer", tx_queue_id, Diligent::QUEUE_PRIORITY_MEDIUM }
        };

        if (comp_queue_id != gfx_queue_id && tx_queue_id != gfx_queue_id) {
            ci.NumImmediateContexts              = 3;
            ci.pImmediateContextInfo             = ctx_info;
            Diligent::IDeviceContext* raw_ctx[3] = {};
            engine_factory->CreateDeviceAndContextsVk( ci, &device, raw_ctx );
            NC_ASSERT( device, "Failed to create a Vulkan device" );
            ctx_gfx.Attach( raw_ctx[0] );
            ctx_comp.Attach( raw_ctx[1] );
            ctx_tx.Attach( raw_ctx[2] );
        } else {
            ci.NumImmediateContexts  = 0;
            ci.pImmediateContextInfo = nullptr;
            engine_factory->CreateDeviceAndContextsVk( ci, &device, &ctx_gfx );
            NC_ASSERT( device, "Failed to create a Vulkan device" );
            ctx_comp = ctx_gfx;
            ctx_tx   = ctx_gfx;
        }

        NC_ASSERT( ctx_gfx, "Failed to create Vulkan device contexts" );
    }

    const Diligent::DeviceFeatures& features = device->GetDeviceInfo().Features;
    if (features.PipelineStatisticsQueries) {
        Diligent::QueryDesc queryDesc;
        queryDesc.Name = "Pipeline statistics query";
        queryDesc.Type = Diligent::QUERY_TYPE_PIPELINE_STATISTICS;
        pipeline_stats_query.reset( new Diligent::ScopedQueryHelper{ device, queryDesc, 2 } );
    }

    if (features.OcclusionQueries) {
        Diligent::QueryDesc queryDesc;
        queryDesc.Name = "Occlusion query";
        queryDesc.Type = Diligent::QUERY_TYPE_OCCLUSION;
        occlusion_query.reset( new Diligent::ScopedQueryHelper{ device, queryDesc, 2 } );
    }

    if (features.TimestampQueries) {
        duration_from_timestamps_query.reset( new Diligent::DurationQueryHelper{ device, 2 } );
    }

    if (features.DurationQueries) {
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
        case GpuQueue::GRAPHICS: {
            auto rid   = ctx_gfx_defer.acquire();
            auto entry = ctx_gfx_defer.get( rid );
            NC_VERIFY( entry );
            *entry = out;
            return rid;
        }
        case GpuQueue::COMPUTE: {
            auto rid   = ctx_comp_defer.acquire();
            auto entry = ctx_comp_defer.get( rid );
            NC_VERIFY( entry );
            *entry = out;
            return rid;
        }
        case GpuQueue::TRANSFER: {
            auto rid   = ctx_tx_defer.acquire();
            auto entry = ctx_tx_defer.get( rid );
            NC_VERIFY( entry );
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
    // NC_LOG_DEBUG_C( log::GRAPHICS, "Setting GPU queue to {}", rtti::get_enum_name( &queue ) );
    active_queue = queue;
}

void DiligentRHI::sync_queue( GpuQueue queue )
{
    auto& ctx = queue == GpuQueue::GRAPHICS ? ctx_gfx : queue == GpuQueue::COMPUTE ? ctx_comp : ctx_tx;
    ctx->WaitForIdle();
}

RID DiligentRHI::swapchain_create( const rhi::SwapChainDesc& desc )
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
    ddesc.Width             = static_cast<Diligent::Uint32>( desc.initial_size.x );
    ddesc.Height            = static_cast<Diligent::Uint32>( desc.initial_size.y );
    ddesc.BufferCount       = static_cast<Diligent::Uint32>( desc.buffer_count );
    ddesc.IsPrimary         = static_cast<Diligent::Bool>( desc.is_primary );
    engine_factory->CreateSwapChainVk( device, ctx_gfx, ddesc, native, &out );
    NC_ASSERT( out, "Failed to create Vulkan swap chain" );

    auto rid    = swapchains.acquire();
    auto handle = swapchains.get( rid );
    NC_VERIFY( handle );
    *handle = std::move( out );

    return rid;
}

Vec2i DiligentRHI::swapchain_get_size( RID sc )
{
    auto entry = swapchains.get( sc );
    NC_VERIFY( entry );
    auto width  = ( *entry )->GetDesc().Width;
    auto height = ( *entry )->GetDesc().Height;
    return Vec2i( width, height );
}

void DiligentRHI::swapchain_set_size( RID sc, Vec2i size )
{
    auto entry = swapchains.get( sc );
    NC_VERIFY( entry );
    auto& desc      = ( *entry )->GetDesc();
    auto new_width  = static_cast<Diligent::Uint32>( size.x );
    auto new_height = static_cast<Diligent::Uint32>( size.y );
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
    NC_VERIFY( entry );
    ( *entry )->Present( sync_interval ? 1 : 0 );
}

void* DiligentRHI::swapchain_get_view( RID sc, rhi::TextureViewType view )
{
    auto entry = swapchains.get( sc );
    NC_VERIFY( entry );
    switch (view) {
        case rhi::TextureViewType::RENDER_TARGET:
            return ( *entry )->GetCurrentBackBufferRTV();
        case rhi::TextureViewType::DEPTH_STENCIL:
            return ( *entry )->GetDepthBufferDSV();
        default:
            break;
    }
    return nullptr;
}

void DiligentRHI::swapchain_destroy( RID target )
{
    NC_LOG_TRACE_C( log::GRAPHICS, "Destroying swap chain (RID: {})", target.value );
    swapchains.release( target );
}

RID DiligentRHI::shader_create( const rhi::ShaderCreateDesc& desc )
{
    Diligent::ShaderCreateInfo ci;
    ci.Desc.Name                       = desc.name.data();
    ci.Desc.ShaderType                 = DiligentTypeHelpers::translate_shader_stage( desc.stage );
    ci.Desc.UseCombinedTextureSamplers = false;
    ci.LoadConstantBufferReflection    = false;
    ci.ByteCode                        = desc.bytecode.data();
    ci.ByteCodeSize                    = desc.bytecode.size_bytes();

    auto handle = shaders.acquire();
    auto shader = shaders.get( handle );
    NC_VERIFY( shader );
    device->CreateShader( ci, &*shader );

    return handle;
}

// ---------------------------------------------------------------------------
// Graphics pipeline
// ---------------------------------------------------------------------------

RID DiligentRHI::gfx_pipeline_create( const rhi::GraphicsPSODesc& desc )
{
    Diligent::RefCntAutoPtr<Diligent::IPipelineState> pso;
    {
        Diligent::GraphicsPipelineStateCreateInfo ci;
        ci.PSODesc.Name = desc.debug_name.c_str();
        if (desc.vertex_shader) {
            auto vs = shaders.get( desc.vertex_shader );
            NC_VERIFY( vs );
            ci.pVS = *vs;
        }
        if (desc.pixel_shader) {
            auto ps = shaders.get( desc.pixel_shader );
            NC_VERIFY( ps );
            ci.pPS = *ps;
        }

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

        DynamicArray<Diligent::LayoutElement> inputs;
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

        DynamicArray<Diligent::IPipelineResourceSignature*> sigs;
        for (auto& rid : desc.resource_signatures) {
            auto entry = res_signatures.get( rid );
            NC_ASSERT( entry && entry->RawPtr(), "A valid resource signature is required for Gfx PSO creation" );
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
                log::GRAPHICS, "PSO '{}' created OK. Layout=[{}] sigs={} RTV='{}'", desc.debug_name, layout_summary,
                desc.resource_signatures.size(), rtti::get_enum_name( &desc.render_target_format )
            );
        } else {
            NC_LOG_ERROR_C( log::GRAPHICS, "FAILED to create PSO '{}'", desc.debug_name );
        }
    }
    NC_VERIFY( pso );

    RID handle = pipelines.acquire();
    auto entry = pipelines.get( handle );
    NC_ASSERT( entry, "Failed to acquire new pipeline state object" );
    *entry = std::move( pso );

    return handle;
}

void DiligentRHI::gfx_pipeline_bind( RID pipeline )
{
    auto entry = pipelines.get( pipeline );
    NC_VERIFY( entry );
    NC_LOG_TRACE_C( log::GRAPHICS, "gfx_pipeline_bind: rid={}", pipeline.value );
    get_active_ctx_()->SetPipelineState( *entry );
}

void DiligentRHI::gfx_pipeline_reload( RID pipeline )
{
    auto entry = pipelines.get( pipeline );
    NC_VERIFY( entry );
    entry->Release();
    pipelines.release( pipeline );
}

// ---------------------------------------------------------------------------

void DiligentRHI::render_target_bind( Span<const void*> rtvs, void* dsv )
{
    auto views    = reinterpret_cast<Diligent::ITextureView**>( const_cast<void**>( rtvs.data() ) );
    auto dsv_view = static_cast<Diligent::ITextureView*>( dsv );
    get_active_ctx_()->SetRenderTargets(
        static_cast<Diligent::Uint32>( rtvs.size() ), views, dsv_view,
        Diligent::RESOURCE_STATE_TRANSITION_MODE_TRANSITION
    );
}

void DiligentRHI::render_target_set_viewport( Span<const Viewport> p_viewports )
{
    DynamicArray<Diligent::Viewport> vps;
    for (auto& vp : p_viewports) {
        vps.push_back( { vp.rect.x, vp.rect.y, vp.rect.w, vp.rect.h, vp.min_depth, vp.max_depth } );
    }
    NC_LOG_TRACE_C(
        log::GRAPHICS, "set_viewport: {} viewports (first: {:.0f},{:.0f} {:.0f}x{:.0f})", vps.size(), vps[0].TopLeftX,
        vps[0].TopLeftY, vps[0].Width, vps[0].Height
    );
    get_active_ctx_()->SetViewports( static_cast<Diligent::Uint32>( vps.size() ), vps.data(), 0, 0 );
}

void DiligentRHI::render_target_set_scissor_rect( Span<const Rect2i> p_rects )
{
    DynamicArray<Diligent::Rect> rects;
    for (auto r : p_rects) {
        rects.push_back(
            Diligent::Rect{
                static_cast<Diligent::Int32>( r.x ), static_cast<Diligent::Int32>( r.y ),
                static_cast<Diligent::Int32>( r.x + r.w ), static_cast<Diligent::Int32>( r.y + r.h )
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

RID DiligentRHI::compute_pipeline_create( const rhi::ComputePSODesc& desc )
{
    auto cs = shaders.get( desc.compute_shader );
    NC_VERIFY( cs );

    Diligent::RefCntAutoPtr<Diligent::IPipelineState> pso;
    {
        Diligent::ComputePipelineStateCreateInfo ci;
        ci.PSODesc.Name                 = desc.debug_name.c_str();
        ci.PSODesc.PipelineType         = Diligent::PIPELINE_TYPE_COMPUTE;
        ci.PSODesc.ImmediateContextMask = ctx_comp->GetDesc().ContextId + 1;
        ci.pCS                          = *cs;

        DynamicArray<Diligent::IPipelineResourceSignature*> sigs;
        for (const auto& sig : desc.resource_signatures) {
            auto entry = res_signatures.get( sig );
            NC_ASSERT( entry && entry->RawPtr(), "A valid resource signature is required for Compute PSO creation" );
            sigs.push_back( entry->RawPtr() );
        }
        if (!sigs.empty()) {
            ci.ppResourceSignatures    = sigs.data();
            ci.ResourceSignaturesCount = static_cast<Diligent::Uint32>( sigs.size() );
        }

        device->CreateComputePipelineState( ci, &pso );

        if (pso) {
            NC_LOG_INFO_C( log::GRAPHICS, "Compute PSO '{}' created OK. sigs={}", desc.debug_name, sigs.size() );
        } else {
            NC_LOG_ERROR_C( log::GRAPHICS, "FAILED to create compute PSO '{}'", desc.debug_name );
        }
    }
    NC_VERIFY( pso );

    RID handle = pipelines.acquire();
    auto entry = pipelines.get( handle );
    NC_ASSERT( entry, "Failed to acquire new compute pipeline state object" );
    *entry = std::move( pso );

    return handle;
}

void DiligentRHI::compute_pipeline_bind( RID pipeline )
{
    auto entry = pipelines.get( pipeline );
    NC_VERIFY( entry );
    NC_LOG_TRACE_C( log::GRAPHICS, "compute_pipeline_bind: rid={}", pipeline.value );
    get_active_ctx_()->SetPipelineState( *entry );
}

void DiligentRHI::compute_dispatch( uint32_t x, uint32_t y, uint32_t z )
{
    NC_LOG_TRACE_C( log::GRAPHICS, "compute dispatch: {}x{}x{}", x, y, z );
    Diligent::DispatchComputeAttribs attrs;
    attrs.ThreadGroupCountX = x;
    attrs.ThreadGroupCountY = y;
    attrs.ThreadGroupCountZ = z;
    get_active_ctx_()->DispatchCompute( attrs );
}

// ---------------------------------------------------------------------------
// Textures
// ---------------------------------------------------------------------------

RID DiligentRHI::texture_create( const rhi::TextureDesc& desc )
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

    Diligent::RefCntAutoPtr<Diligent::ITexture> texture;

    if (desc.dimension == rhi::ResourceDimension::DIM_CUBE) {
        NC_ASSERT( desc.width > 0, "Cube texture width must be > 0" );
        NC_ASSERT( desc.array_size == 6, "Cube texture requires array_size == 6" );
    }

    DynamicArray<Diligent::TextureSubResData> subres;
    for (auto& res : desc.subresources) {
        auto& it  = subres.emplace_back();
        it.pData  = res.pixels;
        it.Stride = static_cast<Diligent::Uint64>( desc.width ) * 4;
    }

    if (subres.empty()) {
        device->CreateTexture( ddesc, nullptr, &texture );
    } else {
        Diligent::TextureData init{};
        init.pSubResources   = subres.data();
        init.NumSubresources = static_cast<Diligent::Uint32>( subres.size() );
        device->CreateTexture( ddesc, &init, &texture );
    }

    RID handle = textures.acquire();
    auto entry = textures.get( handle );
    NC_ASSERT( entry, "Failed acquiring new texture slot" );
    *entry = std::move( texture );

    return handle;
}

void* DiligentRHI::texture_view_get( RID texture, rhi::TextureViewType view )
{
    auto entry = textures.get( texture );
    NC_FAIL_MSG_RETVAL( entry, nullptr, "Texture not found." );

    auto map_view = []( rhi::TextureViewType t ) {
        switch (t) {
            case rhi::TextureViewType::SHADER_RESOURCE:
                return Diligent::TEXTURE_VIEW_SHADER_RESOURCE;
            case rhi::TextureViewType::RENDER_TARGET:
                return Diligent::TEXTURE_VIEW_RENDER_TARGET;
            case rhi::TextureViewType::DEPTH_STENCIL:
                return Diligent::TEXTURE_VIEW_DEPTH_STENCIL;
            case rhi::TextureViewType::UNORDERED_ACCESS:
                return Diligent::TEXTURE_VIEW_UNORDERED_ACCESS;
        }
        return Diligent::TEXTURE_VIEW_UNDEFINED;
    };
    auto result = ( *entry )->GetDefaultView( map_view( view ) );
    if (!result) {
        NC_LOG_ERROR_C(
            log::GRAPHICS, "Texture '{}' GetDefaultView returned null. RID={} view={}", ( *entry )->GetDesc().Name,
            texture.value, rtti::get_enum_name( &view )
        );
    }
    return result;
}

void DiligentRHI::texture_binding_update(
    RID p_texture, RID p_binding, rhi::ShaderStage p_shader_type, rhi::TextureViewType p_view_type, const char* p_name
)
{
    auto srb = res_bindings.get( p_binding );
    NC_VERIFY( srb );

    auto var = ( *srb )->GetVariableByName( DiligentTypeHelpers::translate_shader_stage( p_shader_type ), p_name );
    if (!var) {
        auto& st_enum_t   = static_cast<const rtti::EnumInfo&>( rtti::TypeRegistry::get<rhi::ShaderStage>() );
        auto st_enum_name = st_enum_t.get_name( st_enum_t.get_value( &p_shader_type ) );
        NC_LOG_ERROR_C( log::GRAPHICS, "texture_binding_update: var '{}' NOT FOUND in {} stage", p_name, st_enum_name );
    }
    NC_VERIFY( var );

    auto view = texture_view_get( p_texture, p_view_type );
    NC_VERIFY( view );
    auto view_ptr = reinterpret_cast<Diligent::ITextureView*>( view );
    var->Set( view_ptr, Diligent::SET_SHADER_RESOURCE_FLAG_ALLOW_OVERWRITE );
}

void DiligentRHI::texture_blit( RID texture_src, RID texture_dest, bool to_swapchain )
{
    auto src = textures.get( texture_src );
    NC_VERIFY( src );
    Diligent::ITexture* dest = nullptr;
    if (to_swapchain) {
        auto sc = swapchains.get( texture_dest );
        NC_VERIFY( sc );
        dest = ( *sc )->GetCurrentBackBufferRTV()->GetTexture();
    } else {
        auto dst_ = textures.get( texture_dest );
        NC_VERIFY( dst_ );
        dest = dst_->RawPtr();
    }

    get_active_ctx_()->SetRenderTargets( 0, nullptr, nullptr, Diligent::RESOURCE_STATE_TRANSITION_MODE_TRANSITION );

    Diligent::CopyTextureAttribs attribs{};
    attribs.pSrcTexture              = src->RawPtr();
    attribs.pDstTexture              = dest;
    attribs.SrcTextureTransitionMode = Diligent::RESOURCE_STATE_TRANSITION_MODE_TRANSITION;
    attribs.DstTextureTransitionMode = Diligent::RESOURCE_STATE_TRANSITION_MODE_TRANSITION;
    get_active_ctx_()->CopyTexture( attribs );
}

RID DiligentRHI::sampler_create( const rhi::SamplerDesc& desc )
{
    Diligent::SamplerDesc sd;
    sd.MinFilter = desc.min_filter == rhi::SamplerFilter::NEAREST
                       ? Diligent::FILTER_TYPE_POINT
                       : ( desc.min_filter == rhi::SamplerFilter::ANISOTROPIC ? Diligent::FILTER_TYPE_ANISOTROPIC
                                                                              : Diligent::FILTER_TYPE_LINEAR );
    sd.MagFilter = desc.mag_filter == rhi::SamplerFilter::NEAREST
                       ? Diligent::FILTER_TYPE_POINT
                       : ( desc.mag_filter == rhi::SamplerFilter::ANISOTROPIC ? Diligent::FILTER_TYPE_ANISOTROPIC
                                                                              : Diligent::FILTER_TYPE_LINEAR );
    sd.MipFilter = desc.mip_filter == rhi::SamplerFilter::NEAREST
                       ? Diligent::FILTER_TYPE_POINT
                       : ( desc.mip_filter == rhi::SamplerFilter::ANISOTROPIC ? Diligent::FILTER_TYPE_ANISOTROPIC
                                                                              : Diligent::FILTER_TYPE_LINEAR );

    auto translate_address = []( rhi::TextureAddressMode m ) {
        switch (m) {
            case rhi::TextureAddressMode::WRAP:
                return Diligent::TEXTURE_ADDRESS_WRAP;
            case rhi::TextureAddressMode::MIRROR:
                return Diligent::TEXTURE_ADDRESS_MIRROR;
            case rhi::TextureAddressMode::BORDER:
                return Diligent::TEXTURE_ADDRESS_BORDER;
            case rhi::TextureAddressMode::CLAMP:
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
    NC_VERIFY( entry );
    *entry = std::move( sampler );

    return handle;
}

void DiligentRHI::sampler_binding_update( RID p_sampler, RID p_binding, const char* p_name )
{
    auto srb     = res_bindings.get( p_binding );
    auto sampler = samplers.get( p_sampler );

    NC_VERIFY( srb );
    NC_VERIFY( sampler );

    auto var = ( *srb )->GetVariableByName( Diligent::SHADER_TYPE_PIXEL, p_name );
    if (!var)
        NC_LOG_ERROR_C( log::GRAPHICS, "sampler_binding_update: var '{}' NOT FOUND in PIXEL stage", p_name );
    NC_VERIFY( var );

    var->Set( *sampler );
    NC_LOG_TRACE_C(
        log::GRAPHICS, "sampler_binding_update: var='{}' bound to sampler rid={}", p_name, p_sampler.value
    );
}

// ---------------------------------------------------------------------------
// Buffers
// ---------------------------------------------------------------------------

RID DiligentRHI::buffer_create( const rhi::BufferDesc& p_desc )
{
    Diligent::BufferDesc desc;
    desc.Name           = p_desc.debug_name.data();
    desc.Size           = static_cast<Diligent::Uint32>( p_desc.size );
    desc.BindFlags      = static_cast<Diligent::BIND_FLAGS>( p_desc.bind_mask );
    desc.Usage          = static_cast<Diligent::USAGE>( p_desc.usage );
    desc.CPUAccessFlags = static_cast<Diligent::CPU_ACCESS_FLAGS>( p_desc.access_mask );
    desc.Mode           = static_cast<Diligent::BUFFER_MODE>( p_desc.mode );

    Diligent::BufferData init_data{ p_desc.initial_data, static_cast<Diligent::Uint32>( p_desc.size ) };
    Diligent::RefCntAutoPtr<Diligent::IBuffer> buffer;
    device->CreateBuffer( desc, p_desc.initial_data ? &init_data : nullptr, &buffer );

    RID handle = buffers.acquire();
    auto entry = buffers.get( handle );
    NC_VERIFY( entry );
    *entry = std::move( buffer );

    NC_LOG_DEBUG_C(
        log::GRAPHICS, "buffer_create: name='{}' size={} usage='{}' bind='{}' cpu='{}' rid={}", p_desc.debug_name,
        p_desc.size, rtti::get_enum_name( &p_desc.usage ), rtti::get_enum_name( &p_desc.bind_mask ),
        rtti::get_enum_name( &p_desc.access_mask ), handle.value
    );

    return handle;
}

void* DiligentRHI::buffer_view_get( RID buffer, rhi::BufferViewType view )
{
    auto entry = buffers.get( buffer );
    NC_FAIL_MSG_RETVAL( entry, nullptr, "Buffer not found." );

    auto map_view = []( rhi::BufferViewType t ) {
        switch (t) {
            case rhi::BufferViewType::SHADER_RESOURCE:
                return Diligent::BUFFER_VIEW_SHADER_RESOURCE;
            case rhi::BufferViewType::UNORDERED_ACCESS:
                return Diligent::BUFFER_VIEW_UNORDERED_ACCESS;
        }
        return Diligent::BUFFER_VIEW_UNDEFINED;
    };
    auto result = ( *entry )->GetDefaultView( map_view( view ) );
    if (!result) {
        NC_LOG_ERROR_C(
            log::GRAPHICS, "Buffer '{}' GetDefaultView returned null. RID={} view={}", ( *entry )->GetDesc().Name,
            buffer.value, rtti::get_enum_name( &view )
        );
    }
    return result;
}

void DiligentRHI::buffer_data_write( RID p_buffer, Span<const std::byte> p_src )
{
    auto buffer = buffers.get( p_buffer );
    NC_VERIFY( buffer );
    Diligent::MapHelper<std::byte> map( get_active_ctx_(), *buffer, Diligent::MAP_WRITE, Diligent::MAP_FLAG_DISCARD );
    if (map) {
        memcpy( map, p_src.data(), p_src.size() );
        NC_LOG_TRACE_C( log::GRAPHICS, "buffer_data_write: rid={} size=[{} bytes] OK", p_buffer.value, p_src.size() );
    } else {
        NC_LOG_ERROR_C( log::GRAPHICS, "buffer_data_write: rid={} size={} MAP FAILED", p_buffer.value, p_src.size() );
    }
}

void DiligentRHI::buffer_data_read( RID p_buffer, Span<std::byte> p_dst )
{
    auto buffer = buffers.get( p_buffer );
    NC_VERIFY( buffer );

    Diligent::MapHelper<std::byte> map(
        get_active_ctx_(), *buffer, Diligent::MAP_READ, Diligent::MAP_FLAG_DO_NOT_WAIT
    );
    if (map) {
        memcpy( p_dst.data(), map, p_dst.size() );
        NC_LOG_TRACE_C( log::GRAPHICS, "buffer_data_read: rid={} size=[{} bytes] OK", p_buffer.value, p_dst.size() );
    } else {
        NC_LOG_ERROR_C( log::GRAPHICS, "buffer_data_read: rid={} size={} MAP FAILED", p_buffer.value, p_dst.size() );
    }
}

void DiligentRHI::buffer_blit( RID p_src_buffer, RID p_dst_buffer )
{
    auto src_buffer = buffers.get( p_src_buffer );
    auto dst_buffer = buffers.get( p_dst_buffer );
    NC_VERIFY( src_buffer );
    NC_VERIFY( dst_buffer );

    get_active_ctx_()->CopyBuffer(
        *src_buffer, 0, Diligent::RESOURCE_STATE_TRANSITION_MODE_TRANSITION, *dst_buffer, 0,
        ( *src_buffer )->GetDesc().Size, Diligent::RESOURCE_STATE_TRANSITION_MODE_TRANSITION
    );
}

void DiligentRHI::buffer_binding_update(
    RID p_buffer, RID p_binding, rhi::ShaderStage p_shader_type, rhi::BufferViewType p_view_type, const char* p_name
)
{
    auto srb    = res_bindings.get( p_binding );
    auto buffer = buffers.get( p_buffer );

    NC_VERIFY( srb );
    NC_VERIFY( buffer );

    auto& buf_desc = ( *buffer )->GetDesc();

    auto var = ( *srb )->GetVariableByName( DiligentTypeHelpers::translate_shader_stage( p_shader_type ), p_name );
    if (!var) {
        auto& st_enum_t   = static_cast<const rtti::EnumInfo&>( rtti::TypeRegistry::get<rhi::ShaderStage>() );
        auto st_enum_name = st_enum_t.get_name( st_enum_t.get_value( &p_shader_type ) );
        NC_LOG_ERROR_C( log::GRAPHICS, "buffer_binding_update: var='{}' NOT FOUND in {} stage", p_name, st_enum_name );
    }
    NC_VERIFY( var );

    if (buf_desc.BindFlags == Diligent::BIND_UNIFORM_BUFFER) {
        // apparently constant buffers do not use views and we need to just pass in the buffer itself
        var->Set( *buffer );
    } else {
        auto view = buffer_view_get( p_buffer, p_view_type );
        NC_VERIFY( view );
        auto view_ptr = reinterpret_cast<Diligent::IBufferView*>( view );
        var->Set( view_ptr );
    }
    NC_LOG_TRACE_C( log::GRAPHICS, "buffer_binding_update: var='{}' bound to p_buffer rid={}", p_name, p_buffer.value );
}

void DiligentRHI::buffer_vertices_bind( Span<const RID> p_buffers, uint32_t slot, Span<const uint64_t> offsets )
{
    DynamicArray<Diligent::IBuffer*> buffer_arr;
    for (auto& rid : p_buffers) {
        auto ptr = buffers.get( rid );
        NC_VERIFY( ptr );
        buffer_arr.push_back( ptr->RawPtr() );
    }

    NC_LOG_TRACE_C( log::GRAPHICS, "buffer_vertices_bind: {} buffers at slot={}", p_buffers.size(), slot );
    get_active_ctx_()->SetVertexBuffers(
        slot, static_cast<Diligent::Uint32>( buffer_arr.size() ), buffer_arr.data(), offsets.data(),
        Diligent::RESOURCE_STATE_TRANSITION_MODE_TRANSITION, Diligent::SET_VERTEX_BUFFERS_FLAG_RESET
    );
}

void DiligentRHI::buffer_index_bind( RID buffer, uint32_t offset )
{
    auto buf = buffers.get( buffer );
    NC_VERIFY( buf );
    NC_LOG_TRACE_C( log::GRAPHICS, "buffer_index_bind: rid={} offset={}", buffer.value, offset );
    get_active_ctx_()->SetIndexBuffer( *buf, offset, Diligent::RESOURCE_STATE_TRANSITION_MODE_TRANSITION );
}

// ---------------------------------------------------------------------------

RID DiligentRHI::resource_signature_create( const rhi::ResourceSignatureDesc& desc )
{
    Diligent::PipelineResourceSignatureDesc pdesc{};
    pdesc.BindingIndex = desc.set_idx;
    pdesc.Name         = desc.name.c_str();

    DynamicArray<Diligent::PipelineResourceDesc> rdescs;
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
        NC_LOG_ERROR_C( log::GRAPHICS, "FAILED to create resource signature '{}' (set {})", desc.name, desc.set_idx );
    } else {
        NC_LOG_DEBUG_C(
            log::GRAPHICS, "Resource signature '{}' (set {}) created with {} resources", desc.name, desc.set_idx,
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
        sig.RawPtr(),
        std::format( "Failed to create resource signature '{}' (set {})", desc.name, desc.set_idx ).c_str()
    );

    RID handle = res_signatures.acquire();
    auto entry = res_signatures.get( handle );
    NC_ASSERT( entry, "Failed acquiring new resource signature slot" );
    *entry = std::move( sig );

    return handle;
}

RID DiligentRHI::resource_mapping_create( Span<const rhi::ResourceMappingEntry> p_entries )
{
    DynamicArray<Diligent::ResourceMappingEntry> entries;
    for (auto& p_entry : p_entries) {
        auto& e   = entries.emplace_back();
        e.Name    = p_entry.variable_name;
        e.pObject = map_resource_bind_( p_entry.resource, p_entry.kind );
    }

    Diligent::ResourceMappingCreateInfo res_mapping_desc;
    res_mapping_desc.NumEntries = static_cast<Diligent::Uint32>( entries.size() );
    res_mapping_desc.pEntries   = entries.data();
    Diligent::RefCntAutoPtr<Diligent::IResourceMapping> res_mapping;
    device->CreateResourceMapping( res_mapping_desc, &res_mapping );

    RID handle = res_mappings.acquire();
    auto entry = res_mappings.get( handle );
    NC_VERIFY( entry );
    *entry = std::move( res_mapping );

    return handle;
}

void DiligentRHI::resource_mapping_add_entry(
    RID p_mapping, const rhi::ResourceMappingEntry& p_entry, bool p_is_unique
)
{
    auto mapping = res_mappings.get( p_mapping );
    NC_VERIFY( mapping );
    ( *mapping )
        ->AddResource( p_entry.variable_name, map_resource_bind_( p_entry.resource, p_entry.kind ), p_is_unique );
}

RID DiligentRHI::resource_binding_create( RID p_resource_signature )
{
    auto sig = res_signatures.get( p_resource_signature );
    NC_VERIFY( sig );

    Diligent::RefCntAutoPtr<Diligent::IShaderResourceBinding> srb;
    ( *sig )->CreateShaderResourceBinding( &srb, true );

    RID handle = res_bindings.acquire();
    auto entry = res_bindings.get( handle );
    NC_VERIFY( entry );
    *entry = std::move( srb );

    return handle;
}

void DiligentRHI::resource_binding_update(
    RID p_resource_binding, RID p_resource_mapping, rhi::ShaderStage p_shader_stages
)
{
    auto srb     = res_bindings.get( p_resource_binding );
    auto res_map = res_mappings.get( p_resource_mapping );
    NC_VERIFY( srb );
    NC_VERIFY( res_map );

    ( *srb )->BindResources(
        DiligentTypeHelpers::translate_shader_stage( p_shader_stages ), *res_map,
        Diligent::BIND_SHADER_RESOURCES_ALLOW_OVERWRITE
    );
}

void DiligentRHI::resource_binding_commit( RID resource_binding )
{
    auto srb = res_bindings.get( resource_binding );
    NC_VERIFY( srb );
    NC_LOG_TRACE_C( log::GRAPHICS, "resource_binding_commit: rid={}", resource_binding.value );
    get_active_ctx_()->CommitShaderResources( *srb, Diligent::RESOURCE_STATE_TRANSITION_MODE_TRANSITION );
}

// ---------------------------------------------------------------------------

bool DiligentRHI::is_rid_owned( RID rid )
{
    return ctx_gfx_defer.contains( rid ) || ctx_comp_defer.contains( rid ) || ctx_tx_defer.contains( rid ) ||
           swapchains.contains( rid ) || shaders.contains( rid ) || textures.contains( rid ) ||
           pipelines.contains( rid ) || buffers.contains( rid ) || samplers.contains( rid ) ||
           res_signatures.contains( rid ) || res_mappings.contains( rid ) || res_bindings.contains( rid );
}

bool DiligentRHI::destroy_rid( RID rid )
{
    NC_LOG_DEBUG_C( log::GRAPHICS, "Destroying RID={}", rid.value );
    if (ctx_gfx_defer.release( rid ))
        return true;
    if (ctx_comp_defer.release( rid ))
        return true;
    if (ctx_tx_defer.release( rid ))
        return true;
    if (swapchains.release( rid ))
        return true;
    if (shaders.release( rid ))
        return true;
    if (pipelines.release( rid ))
        return true;
    if (textures.release( rid ))
        return true;
    if (buffers.release( rid ))
        return true;
    if (samplers.release( rid ))
        return true;
    if (res_signatures.release( rid ))
        return true;
    if (res_mappings.release( rid ))
        return true;
    if (res_bindings.release( rid ))
        return true;
    return false;
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
            NC_VERIFY_MSG( ref, err_msg );
            return ref->RawPtr();
        }
        NC_ASSERT( immediate_ctx, err_msg );
        return immediate_ctx.RawPtr();
    };

    switch (active_queue) {
        case GpuQueue::GRAPHICS:
            return resolve( ctx_gfx_defer, ctx_gfx, "Invalid gfx context ref" );
        case GpuQueue::COMPUTE:
            return resolve( ctx_comp_defer, ctx_comp, "Invalid compute context ref" );
        case GpuQueue::TRANSFER:
            return resolve( ctx_tx_defer, ctx_tx, "Invalid transfer context ref" );
    }
    return nullptr;
}

Diligent::IDeviceContext* DiligentRHI::get_imm_ctx_()
{
    switch (active_queue) {
        case GpuQueue::GRAPHICS:
            return ctx_gfx.RawPtr();
        case GpuQueue::COMPUTE:
            return ctx_comp.RawPtr();
        case GpuQueue::TRANSFER:
            return ctx_tx.RawPtr();
    }
    return nullptr;
}

Diligent::IDeviceObject* DiligentRHI::map_resource_bind_( RID p_resource, rhi::ResourceType p_kind )
{
    switch (p_kind) {
        case rhi::ResourceType::TEXTURE_SRV:
        case rhi::ResourceType::TEXTURE_UAV: {
            auto view_type = ( p_kind == rhi::ResourceType::TEXTURE_SRV ) ? rhi::TextureViewType::SHADER_RESOURCE
                                                                          : rhi::TextureViewType::UNORDERED_ACCESS;
            return reinterpret_cast<Diligent::ITextureView*>( texture_view_get( p_resource, view_type ) );
        }
        case rhi::ResourceType::BUFFER_SRV:
        case rhi::ResourceType::BUFFER_UAV: {
            auto view_type = ( p_kind == rhi::ResourceType::BUFFER_UAV ) ? rhi::BufferViewType::UNORDERED_ACCESS
                                                                         : rhi::BufferViewType::SHADER_RESOURCE;
            return reinterpret_cast<Diligent::IBufferView*>( buffer_view_get( p_resource, view_type ) );
        }
        case rhi::ResourceType::CONSTANT_BUFFER: {
            auto buf = buffers.get( p_resource );
            NC_VERIFY_MSG( buf, "Buffer resource to bind is not found" );
            return *buf;
        }
        case rhi::ResourceType::SAMPLER: {
            auto samp = samplers.get( p_resource );
            NC_VERIFY_MSG( samp, "Sampler resource to bind is not found" );
            return *samp;
        }
        default:
            NC_ASSERT( false, "resource_mapping_create: unhandled ResourceType" );
    }
    return nullptr;
}

} // namespace nc
