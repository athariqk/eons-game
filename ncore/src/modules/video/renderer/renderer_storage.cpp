#include <algorithm>
#include <cstring>

#include <ncore/modules/video/renderer/renderer_storage.h>
#include <ncore/modules/video/renderer/vertex_format.h>
#include <ncore/modules/video/rhi.h>
#include <ncore/resources/material_template.h>
#include <ncore/resources/mesh.h>
#include <ncore/resources/shader.h>

namespace nc {

bool RendererStorage::PSOKey::operator==( const PSOKey& o ) const
{
    if (flags != o.flags)
        return false;
    if (vs != o.vs || ps != o.ps)
        return false;
    if (vertex_layout.size() != o.vertex_layout.size())
        return false;
    for (size_t i = 0; i < vertex_layout.size(); ++i) {
        auto& a = vertex_layout[i];
        auto& b = o.vertex_layout[i];
        if (a.location != b.location)
            return false;
        if (a.buffer_slot != b.buffer_slot)
            return false;
        if (a.type != b.type)
            return false;
        if (a.normalized != b.normalized)
            return false;
        if (a.relative_offset != b.relative_offset)
            return false;
        if (a.stride != b.stride)
            return false;
        if (a.frequency != b.frequency)
            return false;
        if (a.instance_step_rate != b.instance_step_rate)
            return false;
        auto* sa = a.hlsl_semantic ? a.hlsl_semantic : "";
        auto* sb = b.hlsl_semantic ? b.hlsl_semantic : "";
        if (std::strcmp( sa, sb ) != 0)
            return false;
    }
    if (resource_signatures.size() != o.resource_signatures.size())
        return false;
    for (size_t i = 0; i < resource_signatures.size(); ++i) {
        if (resource_signatures[i] != o.resource_signatures[i])
            return false;
    }
    return true;
}

RID RendererStorage::get_pipeline_or_create( const PSOKey& key )
{
    auto it = pso_cache.find( key );
    if (it != pso_cache.end()) {
        NC_LOG_DEBUG_C(
            log::GRAPHICS, "get_pipeline_or_create: '{}' cache HIT (rid={})", key.debug_name, it->second.value
        );
        return it->second;
    }

    NC_LOG_DEBUG_C( log::GRAPHICS, "get_pipeline_or_create: '{}' cache MISS -> creating new PSO", key.debug_name );
    NC_VERIFY( rhi );

    GraphicsPSODesc desc;
    desc.debug_name                      = key.debug_name;
    desc.render_target_format            = static_cast<TextureFormat>( ( key.flags >> PSO_RT_FMT_SHIFT ) & 0x7F );
    desc.primitive_topology              = static_cast<PrimitiveTopology>( ( key.flags >> PSO_TOPOLOGY_SHIFT ) & 0xF );
    desc.vs_bytecode                     = key.vs ? key.vs->get_bytecode() : std::span<const uint32_t>();
    desc.ps_bytecode                     = key.ps ? key.ps->get_bytecode() : std::span<const uint32_t>();
    desc.vert_layout                     = key.vertex_layout;
    desc.rasterizer_state.cull           = static_cast<CullMode>( ( key.flags >> PSO_CULL_SHIFT ) & 0x3 );
    desc.rasterizer_state.scissor_enable = ( key.flags & PSO_SCISSOR ) != 0;
    desc.depth_stencil_state.depth_test  = ( key.flags & PSO_DEPTH_TEST ) != 0;
    desc.depth_stencil_state.depth_write = ( key.flags & PSO_DEPTH_WRITE ) != 0;
    auto blend_state                     = static_cast<BlendPreset>( ( key.flags >> PSO_BLEND_SHIFT ) & 0xF );
    auto& rt0                            = desc.blend_state.render_targets[0]; // TODO: multiple render targets
    switch (blend_state) {
        case BlendPreset::OPAQUE:
            rt0.enable = false;
            break;
        case BlendPreset::ALPHA_BLEND:
            rt0.enable    = true;
            rt0.src_color = BlendFactor::SRC_ALPHA;
            rt0.dst_color = BlendFactor::INV_SRC_ALPHA;
            rt0.op_color  = BlendOp::ADD;
            rt0.src_alpha = BlendFactor::ONE;
            rt0.dst_alpha = BlendFactor::INV_SRC_ALPHA;
            rt0.op_alpha  = BlendOp::ADD;
            break;
        case BlendPreset::ALPHA_PREMULTIPLIED:
            rt0.enable    = true;
            rt0.src_color = BlendFactor::ONE;
            rt0.dst_color = BlendFactor::INV_SRC_ALPHA;
            rt0.op_color  = BlendOp::ADD;
            rt0.src_alpha = BlendFactor::ONE;
            rt0.dst_alpha = BlendFactor::INV_SRC_ALPHA;
            rt0.op_alpha  = BlendOp::ADD;
            break;
        case BlendPreset::ADDITIVE:
            rt0.enable    = true;
            rt0.src_color = BlendFactor::SRC_ALPHA;
            rt0.dst_color = BlendFactor::ONE;
            rt0.op_color  = BlendOp::ADD;
            rt0.src_alpha = BlendFactor::ONE;
            rt0.dst_alpha = BlendFactor::ONE;
            rt0.op_alpha  = BlendOp::ADD;
            break;
    }
    desc.multisample_state.count   = static_cast<uint8_t>( ( key.flags >> PSO_MSAA_COUNT_SHIFT ) & 0xF );
    desc.multisample_state.quality = static_cast<uint8_t>( ( key.flags >> PSO_MSAA_QUALITY_SHIFT ) & 0xF );

    auto rid = rhi->gfx_pipeline_create( desc, key.resource_signatures );
    pso_cache.emplace( std::move( key ), rid );
    return rid;
}

size_t RendererStorage::PSOKeyHasher::operator()( const PSOKey& p ) const
{
    auto combine = []( size_t a, size_t b ) -> size_t { return a ^ ( b + 0x9e3779b9 + ( a << 6 ) + ( a >> 2 ) ); };

    size_t h = std::hash<uint64_t>{}( static_cast<uint64_t>( p.flags ) );
    h        = combine( h, reinterpret_cast<size_t>( p.vs ) );
    h        = combine( h, reinterpret_cast<size_t>( p.ps ) );
    for (auto& e : p.vertex_layout) {
        h = combine( h, std::hash<std::string>{}( e.hlsl_semantic ? e.hlsl_semantic : "" ) );
        h = combine( h, e.location );
        h = combine( h, e.buffer_slot );
        h = combine( h, static_cast<size_t>( e.type ) );
        h = combine( h, static_cast<size_t>( e.normalized ) );
        h = combine( h, e.relative_offset );
        h = combine( h, e.stride );
        h = combine( h, static_cast<size_t>( e.frequency ) );
        h = combine( h, e.instance_step_rate );
    }
    for (auto& sig : p.resource_signatures) {
        h = combine( h, std::hash<uint64_t>{}( sig.value ) );
    }
    return h;
}

// ---------------------------------------------------------------------------

RID RendererStorage::material_create( const MaterialTemplate& tmpl )
{
    NC_VERIFY( rhi );

    auto sig_descs = build_resource_signatures_( tmpl );

    DynamicArray<RID> sig_rids;
    sig_rids.reserve( sig_descs.size() );
    for (auto& desc : sig_descs) {
        sig_rids.push_back( rhi->resource_signature_create( desc ) );
    }

    auto key                = get_pso_key_( tmpl );
    key.resource_signatures = sig_rids;
    RID pso                 = get_pipeline_or_create( key );

    RID rid  = materials.acquire();
    auto mat = materials.get( rid );
    NC_VERIFY( mat );

    mat->pso                 = pso;
    mat->resource_signatures = sig_rids;

    mat->srbs.reserve( sig_rids.size() );
    for (auto& sig_rid : sig_rids) {
        mat->srbs.push_back( rhi->resource_binding_create( sig_rid ) );
    }

    SamplerDesc sdesc;
    sdesc.debug_name = tmpl.debug_name + "_Sampler";
    sdesc.mag_filter = SamplerFilter::LINEAR;
    sdesc.min_filter = SamplerFilter::LINEAR;
    sdesc.mip_filter = SamplerFilter::LINEAR;
    sdesc.address_u  = TextureAddressMode::CLAMP;
    sdesc.address_v  = TextureAddressMode::CLAMP;
    sdesc.address_w  = TextureAddressMode::CLAMP;
    mat->sampler     = rhi->sampler_create( sdesc );

    BufferDesc bdesc;
    bdesc.debug_name     = tmpl.debug_name + "_Constants";
    bdesc.size           = sizeof( ShaderConstants );
    bdesc.usage          = ResourceUsage::DYNAMIC;
    bdesc.access_mask    = ResourceAccessFlags::WRITE;
    bdesc.bind_mask      = ResourceBindFlags::UNIFORM_BUFFER;
    mat->constant_buffer = rhi->buffer_create( bdesc );

    for (size_t si = 0; si < sig_descs.size(); si++) {
        auto& sig_desc = sig_descs[si];
        auto& srb      = mat->srbs[si];

        if (!srb.is_valid())
            continue;

        for (const auto& res : sig_desc.resources) {
            if (res.resource_type == ResourceType::SAMPLER && mat->sampler.is_valid()) {
                NC_LOG_DEBUG_C(
                    log::GRAPHICS, "material_create '{}': sampler binding SRB[{}] var='{}'", tmpl.debug_name, si,
                    res.name
                );
                rhi->sampler_update_binding( mat->sampler, srb, res.name.c_str() );
            } else if (res.resource_type == ResourceType::CONSTANT_BUFFER && mat->constant_buffer.is_valid()) {
                NC_LOG_DEBUG_C(
                    log::GRAPHICS, "material_create '{}': CB binding SRB[{}] var='{}'", tmpl.debug_name, si, res.name
                );
                rhi->buffer_update_binding( mat->constant_buffer, srb, res.name.c_str() );
            } else if (res.resource_type == ResourceType::TEXTURE_SRV) {
                NC_LOG_DEBUG_C(
                    log::GRAPHICS, "material_create '{}': texture slot SRB[{}] var='{}'", tmpl.debug_name, si, res.name
                );
                Material::TextureSlot slot;
                slot.name      = res.name;
                slot.srb_index = si;
                mat->texture_slots.push_back( std::move( slot ) );
            }
        }
    }

    return rid;
}

void RendererStorage::material_set_texture( RID handle, RID texture, uint32_t slot )
{
    NC_VERIFY( rhi );

    auto mat = get_material_( handle );
    if (!mat || slot >= mat->texture_slots.size())
        return;

    auto& ts = mat->texture_slots[slot];
    if (ts.srb_index < mat->srbs.size() && mat->srbs[ts.srb_index].is_valid())
        rhi->texture_binding_update( texture, mat->srbs[ts.srb_index], ts.name.c_str() );
}

void RendererStorage::material_bind( RID handle, const ShaderConstants& constants )
{
    NC_VERIFY( rhi );

    auto mat = get_material_( handle );
    NC_VERIFY( mat );

    NC_LOG_TRACE_C(
        log::GRAPHICS, "material_bind: PSO rid={} CB rid={} SRBs={}", mat->pso.value, mat->constant_buffer.value,
        mat->srbs.size()
    );

    rhi->gfx_pipeline_bind( mat->pso );

    rhi->buffer_update( mat->constant_buffer, &constants, sizeof( ShaderConstants ) );
    NC_LOG_TRACE_C(
        log::GRAPHICS, "material_bind: CB updated ({} bytes, [{:.4f}, {:.4f}, {:.4f}, {:.4f} ...])",
        sizeof( constants.ViewProjMatrix ), constants.ViewProjMatrix.data()[0], constants.ViewProjMatrix.data()[1],
        constants.ViewProjMatrix.data()[4], constants.ViewProjMatrix.data()[5]
    );

    for (auto& srb : mat->srbs) {
        if (srb.is_valid())
            rhi->resource_binding_commit( srb );
    }
}

// ---------------------------------------------------------------------------

RID RendererStorage::gpu_mesh_create( const Mesh& mesh )
{
    auto rid    = gpu_meshes.acquire();
    auto result = gpu_meshes.get( rid );
    NC_VERIFY( result );

    auto basename = std::string( mesh.get_class_name() ) + "_" + mesh.filepath + "_";

    NC_LOG_DEBUG_C(
        log::GRAPHICS, "gpu_mesh_create: RID={} vert_count={} idx_count={}", rid.value, mesh.vertex_count(),
        mesh.index_count()
    );

    BufferDesc vdesc;
    vdesc.debug_name   = basename + "VertexBuffer";
    vdesc.size         = mesh.get_vertices().size();
    vdesc.usage        = ResourceUsage::IMMUTABLE;
    vdesc.bind_mask    = ResourceBindFlags::VERTEX_BUFFER;
    vdesc.initial_data = mesh.get_vertices().data();
    result->vertices   = rhi->buffer_create( vdesc );

    BufferDesc idesc;
    idesc.debug_name    = basename + "IndexBuffer";
    idesc.size          = mesh.get_indices().size_bytes();
    idesc.usage         = ResourceUsage::IMMUTABLE;
    idesc.bind_mask     = ResourceBindFlags::INDEX_BUFFER;
    idesc.initial_data  = mesh.get_indices().data();
    result->indices     = rhi->buffer_create( idesc );
    result->index_count = static_cast<uint32_t>( mesh.index_count() );

    return rid;
}

void RendererStorage::gpu_mesh_bind( RID handle )
{
    auto mesh = get_gpu_mesh( handle );
    rhi->vertex_buffers_bind( { &mesh->vertices, 1 }, 0 );
    rhi->index_buffer_bind( mesh->indices, 0 );
}

RendererStorage::GPUMesh* RendererStorage::get_gpu_mesh( RID handle )
{
    auto result = gpu_meshes.get( handle );
    NC_VERIFY( result );
    return result;
}

// ---------------------------------------------------------------------------

void RendererStorage::destroy_rid( RID rid )
{
    if (materials.contains( rid ) || gpu_meshes.contains( rid )) {
        pending_destroys.push_back( rid );
        return;
    }
    rhi->destroy_resource( rid );
}

void RendererStorage::flush_pending_destroys()
{
    for (RID rid : pending_destroys) {
        NC_LOG_DEBUG_C( log::GRAPHICS, "Flushing pending destroys: RID={}", rid.value );
        materials.release( rid );
        gpu_meshes.release( rid );
    }
    pending_destroys.clear();
}

// ---------------------------------------------------------------------------

RendererStorage::Material* RendererStorage::get_material_( RID handle )
{
    auto result = materials.get( handle );
    NC_VERIFY( result );
    return result;
}

RendererStorage::PSOKey RendererStorage::get_pso_key_( const MaterialTemplate& tmpl )
{
    PSOKey key;
    key.flags = static_cast<PSOFlags>(
        ( static_cast<uint64_t>( TextureFormat::RGBA8_UNORM_SRGB ) << PSO_RT_FMT_SHIFT ) |
        ( static_cast<uint64_t>( tmpl.cull_mode ) << PSO_CULL_SHIFT ) | ( tmpl.depth_test ? PSO_DEPTH_TEST : 0 ) |
        ( tmpl.depth_write ? PSO_DEPTH_WRITE : 0 ) | ( static_cast<uint64_t>( tmpl.blend ) << PSO_BLEND_SHIFT ) |
        ( static_cast<uint64_t>( tmpl.multisample_state.count & 0xF ) << PSO_MSAA_COUNT_SHIFT ) |
        ( static_cast<uint64_t>( tmpl.multisample_state.quality & 0xF ) << PSO_MSAA_QUALITY_SHIFT ) | PSO_SCISSOR
    );
    key.vs = tmpl.vs.get();
    key.ps = tmpl.ps.get();
    if (!tmpl.vertex_layout_name.empty()) {
        key.vertex_layout = get_vertex_layout_by_name( tmpl.vertex_layout_name );
        NC_LOG_DEBUG_C(
            log::GRAPHICS, "get_pso_key: '{}' using explicit layout '{}' ({} elements)", tmpl.debug_name,
            tmpl.vertex_layout_name, key.vertex_layout.size()
        );
    } else {
        key.vertex_layout = tmpl.vs ? tmpl.vs->get_desc().vert_layout : VertexLayout{};
        NC_LOG_DEBUG_C(
            log::GRAPHICS, "get_pso_key: '{}' using reflected layout ({} elements)", tmpl.debug_name,
            key.vertex_layout.size()
        );
    }
    for (auto& e : key.vertex_layout) {
        NC_LOG_TRACE_C(
            log::GRAPHICS, "  layout: slot={} loc={} type={} offset={} stride={} semantic='{}'", e.buffer_slot,
            e.location, static_cast<int>( e.type ), e.relative_offset, e.stride, e.hlsl_semantic ? e.hlsl_semantic : ""
        );
    }
    key.debug_name = tmpl.debug_name;
    return key;
}

DynamicArray<ResourceSignatureDesc> RendererStorage::build_resource_signatures_( const MaterialTemplate& tmpl )
{
    HashMap<uint8_t, ResourceSignatureDesc> sets;

    auto add_resource = [&sets]( uint8_t set, PipelineResourceDesc&& res ) {
        auto it = sets.find( set );
        if (it == sets.end()) {
            ResourceSignatureDesc sig;
            sig.name = "signature_set_" + std::to_string( set );
            sig.set  = set;
            it       = sets.emplace( set, std::move( sig ) ).first;
        }
        it->second.resources.push_back( std::move( res ) );
    };

    for (const auto& param : tmpl.params) {
        auto set = static_cast<uint8_t>( param.binding_space );

        NC_LOG_DEBUG_C(
            log::GRAPHICS, "build_resource_signatures: param='{}' type={} set={} fields={}", param.name,
            static_cast<int>( param.resource_type ), set, param.fields.size()
        );

        bool has_texture = false;
        bool has_sampler = false;
        for (const auto& f : param.fields) {
            if (f.type == ShaderValueType::TEXTURE2D)
                has_texture = true;
            else if (f.type == ShaderValueType::SAMPLER)
                has_sampler = true;
        }

        NC_LOG_DEBUG_C( log::GRAPHICS, "  has_texture={} has_sampler={}", has_texture, has_sampler );

        if (has_texture || has_sampler) {
            uint32_t binding = 0;
            for (const auto& f : param.fields) {
                PipelineResourceDesc res;
                res.array_size = 1;
                if (f.type == ShaderValueType::TEXTURE2D) {
                    res.name          = param.name + "." + f.name;
                    res.resource_type = ResourceType::TEXTURE_SRV;
                    res.stage         = ShaderType::PIXEL;
                    NC_LOG_DEBUG_C( log::GRAPHICS, "  -> add texture '{}' set={} stage=PIXEL", res.name, set );
                    add_resource( set, std::move( res ) );
                    binding++;
                } else if (f.type == ShaderValueType::SAMPLER) {
                    res.name          = param.name + "." + f.name;
                    res.resource_type = ResourceType::SAMPLER;
                    res.stage         = ShaderType::PIXEL;
                    NC_LOG_DEBUG_C( log::GRAPHICS, "  -> add sampler '{}' set={} stage=PIXEL", res.name, set );
                    add_resource( set, std::move( res ) );
                    binding++;
                }
            }
            ( void ) binding;
        } else if (param.resource_type == ResourceType::CONSTANT_BUFFER) {
            PipelineResourceDesc res;
            res.name          = param.name;
            res.resource_type = ResourceType::CONSTANT_BUFFER;
            res.stage         = ShaderType::MULTIPLE;
            res.array_size    = 1;
            NC_LOG_DEBUG_C( log::GRAPHICS, "  -> add CB '{}' set={} stage=MULTIPLE", res.name, set );
            add_resource( set, std::move( res ) );
        } else if (param.resource_type == ResourceType::TEXTURE_SRV || param.resource_type == ResourceType::SAMPLER) {
            PipelineResourceDesc res;
            res.name          = param.name;
            res.resource_type = param.resource_type;
            res.stage         = ShaderType::PIXEL;
            res.array_size    = 1;
            NC_LOG_DEBUG_C(
                log::GRAPHICS, "  -> add standalone '{}' type={} set={} stage=PIXEL", res.name,
                static_cast<int>( res.resource_type ), set
            );
            add_resource( set, std::move( res ) );
        }
    }

    NC_LOG_DEBUG_C( log::GRAPHICS, "build_resource_signatures: total sets={}", sets.size() );
    for (auto& [s, sig] : sets) {
        std::string names;
        for (auto& r : sig.resources) {
            if (!names.empty())
                names += ", ";
            names += r.name;
        }
        NC_LOG_DEBUG_C( log::GRAPHICS, "  set {}: resources=[{}]", s, names );
    }

    DynamicArray<ResourceSignatureDesc> result;
    result.reserve( sets.size() );
    for (auto& [set, sig] : sets) {
        result.push_back( std::move( sig ) );
    }
    return result;
}

} // namespace nc
