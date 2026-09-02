#include <algorithm>
#include <cstring>

#include <ncore/resources/material_template.h>
#include <ncore/resources/mesh.h>
#include <ncore/resources/shader.h>
#include <ncore/services/video/renderer/render_storage.h>
#include <ncore/services/video/renderer/vertex_format.h>
#include <ncore/services/video/rhi.h>

namespace nc {

bool RenderStorage::PSOKey::operator==( const PSOKey& o ) const
{
    if (flags != o.flags)
        return false;
    if (vs != o.vs)
        return false;
    if (ps != o.ps)
        return false;
    if (cs != o.cs)
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
    if (res_signatures.size() != o.res_signatures.size())
        return false;
    for (size_t i = 0; i < res_signatures.size(); ++i) {
        if (res_signatures[i] != o.res_signatures[i])
            return false;
    }
    return true;
}

RID RenderStorage::get_gfx_pipeline_or_create( const PSOKey& key )
{
    auto hash = PSOKeyHasher()( key ); // TODO: find a way to cache this somehow

    auto it = pso_cache.find( key );
    if (it != pso_cache.end()) {
        NC_LOG_DEBUG_C(
            log::GRAPHICS, "get_gfx_pipeline_or_create: '{}_{}' cache HIT (rid={})", key.debug_name, hash,
            it->second.value
        );
        return it->second;
    }

    NC_LOG_DEBUG_C(
        log::GRAPHICS, "get_gfx_pipeline_or_create: '{}_{}' cache MISS -> creating new PSO", key.debug_name, hash
    );
    NC_VERIFY( gfx_api );

    rhi::GraphicsPSODesc desc;
    desc.debug_name            = key.debug_name;
    desc.render_target_format  = static_cast<rhi::TextureFormat>( ( key.flags >> PSO_RT_FMT_SHIFT ) & 7 );
    desc.depth_stencil_format  = static_cast<rhi::TextureFormat>( ( key.flags >> PSO_DST_FMT_SHIFT ) & 15 );
    desc.primitive_topology    = static_cast<rhi::PrimitiveTopology>( ( key.flags >> PSO_TOPOLOGY_SHIFT ) & 15 );
    desc.vertex_shader         = key.vs;
    desc.pixel_shader          = key.ps;
    desc.vert_layout           = key.vertex_layout;
    desc.resource_signatures   = key.res_signatures;
    desc.rasterizer_state.cull = static_cast<rhi::CullMode>( ( key.flags >> PSO_CULL_SHIFT ) & 3 );
    desc.rasterizer_state.fill = static_cast<rhi::FillMode>( ( key.flags >> PSO_FILL_SHIFT ) & 3 );
    desc.rasterizer_state.scissor_enable = ( key.flags & PSO_SCISSOR ) != 0;
    desc.depth_stencil_state.depth_test  = ( key.flags & PSO_DEPTH_TEST ) != 0;
    desc.depth_stencil_state.depth_write = ( key.flags & PSO_DEPTH_WRITE ) != 0;
    auto blend_state                     = static_cast<rhi::BlendPreset>( ( key.flags >> PSO_BLEND_SHIFT ) & 15 );
    auto& rt0                            = desc.blend_state.render_targets[0]; // TODO: multiple render targets
    switch (blend_state) {
        case rhi::BlendPreset::OPAQUE:
            rt0.enable = false;
            break;
        case rhi::BlendPreset::ALPHA_BLEND:
            rt0.enable    = true;
            rt0.src_color = rhi::BlendFactor::SRC_ALPHA;
            rt0.dst_color = rhi::BlendFactor::INV_SRC_ALPHA;
            rt0.op_color  = rhi::BlendOp::ADD;
            rt0.src_alpha = rhi::BlendFactor::ONE;
            rt0.dst_alpha = rhi::BlendFactor::INV_SRC_ALPHA;
            rt0.op_alpha  = rhi::BlendOp::ADD;
            break;
        case rhi::BlendPreset::ALPHA_PREMULTIPLIED:
            rt0.enable    = true;
            rt0.src_color = rhi::BlendFactor::ONE;
            rt0.dst_color = rhi::BlendFactor::INV_SRC_ALPHA;
            rt0.op_color  = rhi::BlendOp::ADD;
            rt0.src_alpha = rhi::BlendFactor::ONE;
            rt0.dst_alpha = rhi::BlendFactor::INV_SRC_ALPHA;
            rt0.op_alpha  = rhi::BlendOp::ADD;
            break;
        case rhi::BlendPreset::ADDITIVE:
            rt0.enable    = true;
            rt0.src_color = rhi::BlendFactor::SRC_ALPHA;
            rt0.dst_color = rhi::BlendFactor::ONE;
            rt0.op_color  = rhi::BlendOp::ADD;
            rt0.src_alpha = rhi::BlendFactor::ONE;
            rt0.dst_alpha = rhi::BlendFactor::ONE;
            rt0.op_alpha  = rhi::BlendOp::ADD;
            break;
    }
    desc.multisample_state.count   = static_cast<uint8_t>( ( key.flags >> PSO_MSAA_COUNT_SHIFT ) & 15 );
    desc.multisample_state.quality = static_cast<uint8_t>( ( key.flags >> PSO_MSAA_QUALITY_SHIFT ) & 15 );

    auto rid = gfx_api->gfx_pipeline_create( desc );
    pso_cache.emplace( key, rid );
    return rid;
}

RID RenderStorage::get_compute_pipeline_or_create( const PSOKey& key )
{
    auto hash = PSOKeyHasher()( key ); // TODO: find a way to cache this somehow

    auto it = pso_cache.find( key );
    if (it != pso_cache.end()) {
        NC_LOG_DEBUG_C(
            log::GRAPHICS, "get_compute_pipeline_or_create: '{}_{}' cache HIT (rid={})", key.debug_name, hash,
            it->second.value
        );
        return it->second;
    }

    NC_LOG_DEBUG_C(
        log::GRAPHICS, "get_compute_pipeline_or_create: '{}_{}' cache MISS -> creating new PSO", key.debug_name, hash
    );
    NC_VERIFY( gfx_api );

    rhi::ComputePSODesc desc{};
    desc.debug_name          = key.debug_name;
    desc.compute_shader      = key.cs;
    desc.resource_signatures = key.res_signatures;

    auto rid = gfx_api->compute_pipeline_create( desc );
    pso_cache.emplace( key, rid );
    return rid;
}

RID RenderStorage::resource_set_create(
    const Shader& p_shader, rhi::SetIndex p_set_idx, Span<const rhi::ResourceMappingEntry> p_resources
)
{
    auto sig_key = ResSignatureKey{ &p_shader, p_set_idx };
    RID sig_rid;
    if (auto it = res_signature_cache.find( sig_key ); it != res_signature_cache.end()) {
        // use cache
        sig_rid = it->second;
    } else {
        rhi::ResourceSignatureDesc signature;
        signature.name    = p_shader.filepath + "_Signature" + String( std::to_string( p_set_idx ) );
        signature.set_idx = p_set_idx;

        for (const auto& param : p_shader.get_params()) {
            if (param.binding_space != p_set_idx)
                continue; // not part of our wanted set, skip

            const char* res_type_name = rtti::get_enum_name( &param.resource_type );

            NC_LOG_DEBUG_C(
                log::GRAPHICS, "build_res_signature_desc: param='{}' type={} set={} fields={}", param.name,
                res_type_name, p_set_idx, param.fields.size()
            );

            bool has_texture = false;
            bool has_sampler = false;
            for (const auto& f : param.fields) {
                if (f.type & ( rhi::ShaderValueType::TEXTURE_2D | rhi::ShaderValueType::TEXTURE_CUBED ))
                    has_texture = true;
                else if (f.type == rhi::ShaderValueType::SAMPLER)
                    has_sampler = true;
            }

            NC_LOG_DEBUG_C( log::GRAPHICS, "  has_texture={} has_sampler={}", has_texture, has_sampler );

            const char* stage_name = rtti::get_enum_name( &param.stage_mask );

            if (has_texture || has_sampler) {
                uint32_t binding = 0;
                for (const auto& f : param.fields) {
                    rhi::PipelineResourceDesc res;
                    res.array_size = 1;
                    if (f.type & ( rhi::ShaderValueType::TEXTURE_2D | rhi::ShaderValueType::TEXTURE_CUBED )) {
                        res.name          = param.name + "." + f.name;
                        res.resource_type = rhi::ResourceType::TEXTURE_SRV;
                        res.stage         = param.stage_mask;
                        NC_LOG_DEBUG_C(
                            log::GRAPHICS, "  -> add texture '{}' set={} stage={}", res.name, p_set_idx, stage_name
                        );
                        signature.resources.push_back( std::move( res ) );
                        binding++;
                    } else if (f.type == rhi::ShaderValueType::SAMPLER) {
                        res.name          = param.name + "." + f.name;
                        res.resource_type = rhi::ResourceType::SAMPLER;
                        res.stage         = param.stage_mask;
                        NC_LOG_DEBUG_C(
                            log::GRAPHICS, "  -> add sampler '{}' set={} stage={}", res.name, p_set_idx, stage_name
                        );
                        signature.resources.push_back( std::move( res ) );
                        binding++;
                    }
                }
                ( void ) binding;
            } else {
                rhi::PipelineResourceDesc res;
                res.name          = param.name;
                res.resource_type = param.resource_type;
                res.stage         = param.stage_mask;
                res.array_size    = 1;
                NC_LOG_DEBUG_C(
                    log::GRAPHICS, "  -> add standalone '{}' type={} set={} stage={}", res.name, res_type_name,
                    p_set_idx, stage_name
                );
                signature.resources.push_back( std::move( res ) );
            }
        }

        std::string names;
        for (auto& r : signature.resources) {
            if (!names.empty())
                names += ", ";
            names += r.name;
        }
        NC_LOG_DEBUG_C( log::GRAPHICS, "  set {}: resources=[{}]", p_set_idx, names );

        sig_rid = gfx_api->resource_signature_create( signature );
        res_signature_cache.emplace( sig_key, sig_rid );
    }

    RID binding_rid     = gfx_api->resource_binding_create( sig_rid );
    RID res_mapping_rid = gfx_api->resource_mapping_create( p_resources );
    gfx_api->resource_binding_update( binding_rid, res_mapping_rid, p_shader.get_stage_flags() );

    auto rid = resource_sets.acquire();
    auto set = resource_sets.get( rid );
    NC_VERIFY( set );

    set->signature = sig_rid;
    set->mapping   = res_mapping_rid;
    set->binding   = binding_rid;
    set->set_idx   = p_set_idx;
    for (auto& r : p_resources)
        set->slot_kinds.push_back( r.kind );

    return rid;
}

void RenderStorage::resource_set_bind( RID p_resource_set )
{
    auto* set = resource_sets.get( p_resource_set );
    NC_VERIFY( set );
    gfx_api->resource_binding_commit( set->binding );
}

size_t RenderStorage::PSOKeyHasher::operator()( const PSOKey& p ) const
{
    auto combine = []( size_t a, size_t b ) -> size_t { return a ^ ( b + 0x9e3779b9 + ( a << 6 ) + ( a >> 2 ) ); };

    size_t h = std::hash<uint64_t>{}( static_cast<uint64_t>( p.flags ) );
    h        = combine( h, std::hash<uint64_t>{}( p.vs.value ) );
    h        = combine( h, std::hash<uint64_t>{}( p.ps.value ) );
    h        = combine( h, std::hash<uint64_t>{}( p.cs.value ) );
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
    for (auto& sig : p.res_signatures) {
        h = combine( h, std::hash<uint64_t>{}( sig.value ) );
    }
    return h;
}

// ---------------------------------------------------------------------------

RID RenderStorage::material_create( const MaterialTemplate& tmpl )
{
    NC_VERIFY( gfx_api );

    rhi::SamplerDesc sdesc;
    sdesc.debug_name = tmpl.debug_name + "_Sampler";
    sdesc.mag_filter = rhi::SamplerFilter::LINEAR;
    sdesc.min_filter = rhi::SamplerFilter::LINEAR;
    sdesc.mip_filter = rhi::SamplerFilter::LINEAR;
    sdesc.address_u  = rhi::TextureAddressMode::CLAMP;
    sdesc.address_v  = rhi::TextureAddressMode::CLAMP;
    sdesc.address_w  = rhi::TextureAddressMode::CLAMP;
    auto sampler     = gfx_api->sampler_create( sdesc );

    rhi::BufferDesc bdesc;
    bdesc.debug_name  = tmpl.debug_name + "_Constants";
    bdesc.size        = sizeof( ShaderConstants );
    bdesc.usage       = rhi::ResourceUsage::DYNAMIC;
    bdesc.access_mask = rhi::ResourceAccessFlags::WRITE;
    bdesc.bind_mask   = rhi::ResourceBindFlags::UNIFORM_BUFFER;
    auto cbuffer      = gfx_api->buffer_create( bdesc );

    RID rid  = materials.acquire();
    auto mat = materials.get( rid );
    NC_VERIFY( mat );

    mat->sampler = sampler;
    mat->cbuffer = cbuffer;
    mat->shader  = tmpl.shader.get();

    ensure_fallback_texture_();

    HashMap<rhi::SetIndex, DynamicArray<ShaderParamInfo>> sets;
    for (const auto& param : tmpl.shader->get_params()) {
        auto set_idx = static_cast<rhi::SetIndex>( param.binding_space );
        sets[set_idx].push_back( param );
    }

    for (auto& [space, params] : sets) {
        DynamicArray<rhi::ResourceMappingEntry> entries;
        DynamicArray<String> expanded_names;
        for (auto& param : params) {
            bool has_texture = false;
            bool has_sampler = false;
            for (const auto& f : param.fields) {
                if (f.type & ( rhi::ShaderValueType::TEXTURE_2D | rhi::ShaderValueType::TEXTURE_CUBED ))
                    has_texture = true;
                else if (f.type == rhi::ShaderValueType::SAMPLER)
                    has_sampler = true;
            }

            if (has_texture || has_sampler) {
                for (const auto& f : param.fields) {
                    expanded_names.push_back( param.name + "." + f.name );
                    const char* name = expanded_names.back().c_str();
                    if (f.type & ( rhi::ShaderValueType::TEXTURE_2D | rhi::ShaderValueType::TEXTURE_CUBED )) {
                        entries.push_back(
                            rhi::ResourceMappingEntry{
                                .variable_name = name, .kind = rhi::ResourceType::TEXTURE_SRV, .resource = white_texture
                            }
                        );
                    } else if (f.type == rhi::ShaderValueType::SAMPLER) {
                        entries.push_back(
                            rhi::ResourceMappingEntry{
                                .variable_name = name, .kind = rhi::ResourceType::SAMPLER, .resource = mat->sampler
                            }
                        );
                    }
                }
            } else if (param.resource_type == rhi::ResourceType::SAMPLER) {
                entries.push_back(
                    rhi::ResourceMappingEntry{
                        .variable_name = param.name.c_str(), .kind = param.resource_type, .resource = mat->sampler
                    }
                );
            } else if (param.resource_type == rhi::ResourceType::CONSTANT_BUFFER) {
                entries.push_back(
                    rhi::ResourceMappingEntry{
                        .variable_name = param.name.c_str(), .kind = param.resource_type, .resource = mat->cbuffer
                    }
                );
            } else if (param.resource_type == rhi::ResourceType::TEXTURE_SRV) {
                entries.push_back(
                    rhi::ResourceMappingEntry{
                        .variable_name = param.name.c_str(), .kind = param.resource_type, .resource = white_texture
                    }
                ); // placeholder
            }
        }

        RID uset = resource_set_create( *tmpl.shader, space, entries );
        mat->resource_sets.push_back( uset );

        for (auto& e : entries) {
            if (e.kind == rhi::ResourceType::TEXTURE_SRV) {
                NC_LOG_DEBUG_C(
                    log::GRAPHICS, "material_create '{}': texture slot SetIndex[{}] var='{}'", tmpl.debug_name, space,
                    e.variable_name
                );
                Material::TextureSlot slot;
                slot.name            = e.variable_name;
                slot.uniform_set_idx = mat->resource_sets.size() - 1;
                mat->texture_slots.push_back( std::move( slot ) );
            }
        }
    }

    auto key = get_pso_key_( tmpl );
    for (auto& e : mat->resource_sets) {
        auto set = resource_sets.get( e );
        NC_VERIFY( set );
        key.res_signatures.push_back( set->signature );
    }
    RID pso = get_gfx_pipeline_or_create( key );

    mat->pso     = pso;
    mat->pso_key = key;

    return rid;
}

void RenderStorage::material_set_texture( RID handle, RID texture, uint32_t slot )
{
    NC_VERIFY( gfx_api );

    ensure_fallback_texture_();
    if (!texture || !gfx_api->is_rid_owned( texture )) {
        texture = white_texture; // fallback texture.
    }

    auto mat = get_material_( handle );
    if (!mat || slot >= mat->texture_slots.size())
        return;

    auto& ts = mat->texture_slots[slot];
    if (ts.uniform_set_idx >= mat->resource_sets.size())
        return;
    auto& uniform_set_rid = mat->resource_sets[ts.uniform_set_idx];
    if (!uniform_set_rid)
        return;

    auto uniform_set = resource_sets.get( uniform_set_rid );
    NC_VERIFY( uniform_set );

    gfx_api->resource_mapping_add_entry(
        uniform_set->mapping, rhi::ResourceMappingEntry{ ts.name.c_str(), rhi::ResourceType::TEXTURE_SRV, texture },
        false
    );
    gfx_api->resource_binding_update( uniform_set->binding, uniform_set->mapping, mat->shader->get_stage_flags() );
}

void RenderStorage::material_set_draw_mode( RID handle, rhi::FillMode mode )
{
    NC_VERIFY( gfx_api );

    auto mat = get_material_( handle );
    NC_VERIFY( mat );

    auto& key = mat->pso_key;
    key.flags =
        static_cast<PSOFlags>( ( key.flags & ~PSO_FILL_MASK ) | ( static_cast<uint64_t>( mode ) << PSO_FILL_SHIFT ) );
    mat->pso = get_gfx_pipeline_or_create( key );
}

void RenderStorage::material_bind( RID handle, const ShaderConstants& constants )
{
    NC_VERIFY( gfx_api );

    auto mat = get_material_( handle );
    NC_VERIFY( mat );

    NC_LOG_TRACE_C(
        log::GRAPHICS, "material_bind: PSO rid={} CB rid={} UniformSets={}", mat->pso.value, mat->cbuffer.value,
        mat->resource_sets.size()
    );

    gfx_api->gfx_pipeline_bind( mat->pso );
    gfx_api->buffer_data_write(
        mat->cbuffer, { reinterpret_cast<const std::byte*>( &constants ), sizeof( ShaderConstants ) }
    );
    for (auto& e : mat->resource_sets) {
        resource_set_bind( e );
    }
}

// ---------------------------------------------------------------------------

RID RenderStorage::gpu_mesh_create( const Mesh& mesh )
{
    auto rid    = gpu_meshes.acquire();
    auto result = gpu_meshes.get( rid );
    NC_VERIFY( result );

    auto basename = std::format( "{}_{}", mesh.get_class_name(), rid.value );

    NC_LOG_DEBUG_C(
        log::GRAPHICS, "gpu_mesh_create: RID={} vert_count={} idx_count={}", rid.value, mesh.vertex_count(),
        mesh.index_count()
    );

    rhi::BufferDesc vdesc;
    vdesc.debug_name   = basename + "_VBO";
    vdesc.size         = mesh.get_vertices().size();
    vdesc.initial_data = mesh.get_vertices().data();
    vdesc.usage        = rhi::ResourceUsage::IMMUTABLE;
    vdesc.bind_mask    = rhi::ResourceBindFlags::VERTEX_BUFFER;
    result->vertices   = gfx_api->buffer_create( vdesc );

    rhi::BufferDesc idesc;
    idesc.debug_name    = basename + "_IBO";
    idesc.size          = mesh.get_indices().size_bytes();
    idesc.initial_data  = mesh.get_indices().data();
    idesc.usage         = rhi::ResourceUsage::IMMUTABLE;
    idesc.bind_mask     = rhi::ResourceBindFlags::INDEX_BUFFER;
    result->indices     = gfx_api->buffer_create( idesc );
    result->index_count = static_cast<uint32_t>( mesh.index_count() );

    return rid;
}

void RenderStorage::gpu_mesh_bind( RID handle )
{
    auto mesh = get_gpu_mesh( handle );
    gfx_api->buffer_vertices_bind( { &mesh->vertices, 1 }, 0 );
    gfx_api->buffer_index_bind( mesh->indices, 0 );
}

RenderStorage::GPUMesh* RenderStorage::get_gpu_mesh( RID handle )
{
    auto result = gpu_meshes.get( handle );
    NC_VERIFY( result );
    return result;
}

// ---------------------------------------------------------------------------

bool RenderStorage::is_rid_owned( RID rid )
{
    return materials.contains( rid ) || gpu_meshes.contains( rid );
}

bool RenderStorage::destroy_rid( RID rid )
{
    if (materials.contains( rid ) || gpu_meshes.contains( rid )) {
        pending_destroys.push_back( rid );
        return true;
    }
    return false;
}

void RenderStorage::flush_pending_destroys()
{
    for (RID rid : pending_destroys) {
        NC_LOG_DEBUG_C( log::GRAPHICS, "Flushing pending destroys: RID={}", rid.value );
        materials.release( rid );
        gpu_meshes.release( rid );
    }
    pending_destroys.clear();
}

void RenderStorage::set_graphics_api( IRHI* p_gfx_api )
{
    gfx_api = p_gfx_api;
}

// ---------------------------------------------------------------------------

void RenderStorage::ensure_fallback_texture_()
{
    if (gfx_api->is_rid_owned( white_texture ))
        return;

    uint8_t pixels[4] = { 255, 255, 255, 255 };
    rhi::TextureDesc desc{};
    desc.debug_name  = "WhiteTexture";
    desc.format      = rhi::TextureFormat::RGBA8_UNORM_SRGB;
    desc.dimension   = rhi::ResourceDimension::DIM_2D;
    desc.usage       = rhi::ResourceUsage::DYNAMIC;
    desc.access_mask = rhi::ResourceAccessFlags::WRITE;
    desc.width       = 1;
    desc.height      = 1;
    desc.subresources.emplace_back( pixels );
    white_texture = gfx_api->texture_create( desc );
}

RenderStorage::Material* RenderStorage::get_material_( RID handle )
{
    auto result = materials.get( handle );
    NC_VERIFY( result );
    return result;
}

RenderStorage::PSOKey RenderStorage::get_pso_key_( const MaterialTemplate& tmpl )
{
    PSOKey key;

    key.flags = static_cast<PSOFlags>(
        ( static_cast<uint64_t>( rhi::TextureFormat::RGBA8_UNORM_SRGB ) << PSO_RT_FMT_SHIFT ) |
        ( static_cast<uint64_t>( tmpl.cull_mode ) << PSO_CULL_SHIFT ) |
        ( static_cast<uint64_t>( tmpl.fill_mode ) << PSO_FILL_SHIFT ) | ( tmpl.depth_test ? PSO_DEPTH_TEST : 0 ) |
        ( tmpl.depth_write ? PSO_DEPTH_WRITE : 0 ) | ( static_cast<uint64_t>( tmpl.blend ) << PSO_BLEND_SHIFT ) |
        ( static_cast<uint64_t>( tmpl.multisample_state.count & 0xF ) << PSO_MSAA_COUNT_SHIFT ) |
        ( static_cast<uint64_t>( tmpl.multisample_state.quality & 0xF ) << PSO_MSAA_QUALITY_SHIFT ) | PSO_SCISSOR
    );
    bool require_depth = tmpl.depth_test || tmpl.depth_write;
    if (require_depth) {
        key.flags = static_cast<PSOFlags>(
            key.flags | static_cast<uint64_t>( rhi::TextureFormat::D32_FLOAT ) << PSO_DST_FMT_SHIFT
        );
    } else {
        key.flags = static_cast<PSOFlags>(
            key.flags | static_cast<uint64_t>( rhi::TextureFormat::UNKNOWN ) << PSO_DST_FMT_SHIFT
        );
    }

    if (auto vs_desc = tmpl.shader->get_stage_desc( rhi::ShaderStage::VERTEX )) {
        key.vs = gfx_api->shader_create(
            rhi::ShaderCreateDesc{
                .name = key.debug_name + "_VS", .stage = vs_desc->stage, .bytecode = vs_desc->bytecode
            }
        );
    }
    if (auto ps_desc = tmpl.shader->get_stage_desc( rhi::ShaderStage::PIXEL )) {
        key.ps = gfx_api->shader_create(
            rhi::ShaderCreateDesc{
                .name = key.debug_name + "_PS", .stage = ps_desc->stage, .bytecode = ps_desc->bytecode
            }
        );
    }

    if (!tmpl.vertex_layout_name.empty()) {
        key.vertex_layout = get_vertex_layout_by_name( std::string( tmpl.vertex_layout_name.c_str() ) );
        NC_LOG_DEBUG_C(
            log::GRAPHICS, "get_pso_key: '{}' using explicit layout '{}' ({} elements)", tmpl.debug_name,
            tmpl.vertex_layout_name, key.vertex_layout.size()
        );
    } else {
        key.vertex_layout =
            tmpl.shader ? tmpl.shader->get_stage_desc( rhi::ShaderStage::VERTEX )->vert_layout : rhi::VertexLayout{};
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

bool RenderStorage::ResSignatureKey::operator==( const ResSignatureKey& o ) const
{
    if (shader != o.shader)
        return false;
    if (set_idx != o.set_idx)
        return false;
    return true;
}

std::size_t RenderStorage::ResSignatureKeyHasher::operator()( const ResSignatureKey& p ) const
{
    auto combine = []( size_t a, size_t b ) -> size_t { return a ^ ( b + 0x9e3779b9 + ( a << 6 ) + ( a >> 2 ) ); };
    size_t h     = std::hash<uint8_t>{}( p.set_idx );
    h            = combine( h, reinterpret_cast<size_t>( p.shader ) );
    return h;
}

} // namespace nc
