#include <algorithm>
#include <cstring>

#include <ncore/resources/resource.h>
#include <ncore/services/video/gfx_interface.h>
#include <ncore/services/video/renderer/render_storage.h>
#include <ncore/services/video/renderer/vertex_format.h>
#include <ncore/utils/log.h>

namespace nc {

// ---------------------------------------------------------------------------

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
        // auto* sa = a.hlsl_semantic ? a.hlsl_semantic : "";
        // auto* sb = b.hlsl_semantic ? b.hlsl_semantic : "";
        // if (std::strcmp( sa, sb ) != 0)
        //     return false;
    }
    if (res_signatures.size() != o.res_signatures.size())
        return false;
    for (size_t i = 0; i < res_signatures.size(); ++i) {
        if (res_signatures[i] != o.res_signatures[i])
            return false;
    }
    return true;
}

RID RenderStorage::get_render_pipeline_or_create( const PSOKey& key )
{
    auto hash = PSOKeyHasher()( key ); // TODO: find a way to cache this somehow

    auto it = pso_cache.find( key );
    if (it != pso_cache.end()) {
        NC_LOG_DEBUG_C(
            log::GRAPHICS, "get_render_pipeline_or_create: '{}_{}' cache HIT (rid={})", key.debug_name, hash,
            it->second.value
        );
        return it->second;
    }

    NC_LOG_DEBUG_C(
        log::GRAPHICS, "get_render_pipeline_or_create: '{}_{}' cache MISS -> creating new render PSO", key.debug_name,
        hash
    );
    NC_VERIFY( gfx_api );

    gfx::RenderPipelineDesc desc;
    desc.debug_name            = key.debug_name;
    desc.render_target_format  = static_cast<gfx::TextureFormat>( ( key.flags >> PSO_RT_FMT_SHIFT ) & 7 );
    desc.depth_stencil_format  = static_cast<gfx::TextureFormat>( ( key.flags >> PSO_DST_FMT_SHIFT ) & 15 );
    desc.primitive_topology    = static_cast<gfx::PrimitiveTopology>( ( key.flags >> PSO_TOPOLOGY_SHIFT ) & 15 );
    desc.vertex_shader         = key.vs;
    desc.pixel_shader          = key.ps;
    desc.vert_layout           = key.vertex_layout;
    desc.resource_signatures   = key.res_signatures;
    desc.rasterizer_state.cull = static_cast<gfx::CullMode>( ( key.flags >> PSO_CULL_SHIFT ) & 3 );
    desc.rasterizer_state.fill = static_cast<gfx::FillMode>( ( key.flags >> PSO_FILL_SHIFT ) & 3 );
    desc.rasterizer_state.scissor_enable = ( key.flags & PSO_SCISSOR ) != 0;
    desc.depth_stencil_state.depth_test  = ( key.flags & PSO_DEPTH_TEST ) != 0;
    desc.depth_stencil_state.depth_write = ( key.flags & PSO_DEPTH_WRITE ) != 0;
    auto blend_state                     = static_cast<gfx::BlendPreset>( ( key.flags >> PSO_BLEND_SHIFT ) & 15 );
    auto& rt0                            = desc.blend_state.render_targets[0]; // TODO: multiple render targets
    switch (blend_state) {
        case gfx::BlendPreset::OPAQUE:
            rt0.enable = false;
            break;
        case gfx::BlendPreset::ALPHA_BLEND:
            rt0.enable    = true;
            rt0.src_color = gfx::BlendFactor::SRC_ALPHA;
            rt0.dst_color = gfx::BlendFactor::INV_SRC_ALPHA;
            rt0.op_color  = gfx::BlendOp::ADD;
            rt0.src_alpha = gfx::BlendFactor::ONE;
            rt0.dst_alpha = gfx::BlendFactor::INV_SRC_ALPHA;
            rt0.op_alpha  = gfx::BlendOp::ADD;
            break;
        case gfx::BlendPreset::ALPHA_PREMULTIPLIED:
            rt0.enable    = true;
            rt0.src_color = gfx::BlendFactor::ONE;
            rt0.dst_color = gfx::BlendFactor::INV_SRC_ALPHA;
            rt0.op_color  = gfx::BlendOp::ADD;
            rt0.src_alpha = gfx::BlendFactor::ONE;
            rt0.dst_alpha = gfx::BlendFactor::INV_SRC_ALPHA;
            rt0.op_alpha  = gfx::BlendOp::ADD;
            break;
        case gfx::BlendPreset::ADDITIVE:
            rt0.enable    = true;
            rt0.src_color = gfx::BlendFactor::SRC_ALPHA;
            rt0.dst_color = gfx::BlendFactor::ONE;
            rt0.op_color  = gfx::BlendOp::ADD;
            rt0.src_alpha = gfx::BlendFactor::ONE;
            rt0.dst_alpha = gfx::BlendFactor::ONE;
            rt0.op_alpha  = gfx::BlendOp::ADD;
            break;
    }
    desc.multisample_state.count   = static_cast<uint8_t>( ( key.flags >> PSO_MSAA_COUNT_SHIFT ) & 15 );
    desc.multisample_state.quality = static_cast<uint8_t>( ( key.flags >> PSO_MSAA_QUALITY_SHIFT ) & 15 );

    auto rid = gfx_api->render_pipeline_create( desc );
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
        log::GRAPHICS, "get_compute_pipeline_or_create: '{}_{}' cache MISS -> creating new compute PSO", key.debug_name,
        hash
    );
    NC_VERIFY( gfx_api );

    gfx::ComputePSODesc desc{};
    desc.debug_name          = key.debug_name;
    desc.compute_shader      = key.cs;
    desc.resource_signatures = key.res_signatures;

    auto rid = gfx_api->compute_pipeline_create( desc );
    pso_cache.emplace( key, rid );
    return rid;
}

static uint32_t shader_var_type_size( gfx::ParameterType type )
{
    using T = gfx::ParameterType;
    if (any( type & T::MAT4 ))
        return 64;
    if (any( type & T::MAT3 ))
        return 48;
    uint32_t components = 1;
    if (any( type & T::VEC4 ))
        components = 4;
    else if (any( type & T::VEC3 ))
        components = 3;
    else if (any( type & T::VEC2 ))
        components = 2;
    return components * 4;
}

static bool is_bindable_resource( gfx::ResourceType type )
{
    switch (type) {
        case gfx::ResourceType::CONSTANT_BUFFER:
        case gfx::ResourceType::TEXTURE_SRV:
        case gfx::ResourceType::TEXTURE_UAV:
        case gfx::ResourceType::BUFFER_SRV:
        case gfx::ResourceType::BUFFER_UAV:
        case gfx::ResourceType::SAMPLER:
            return true;
        case gfx::ResourceType::VARYING_INPUT:
        case gfx::ResourceType::UNKNOWN:
            return false;
    }
}

RID RenderStorage::resource_set_create(
    const Shader& p_shader, gfx::BindingSet p_set, Span<const gfx::ResourceMappingEntry> p_resources
)
{
    NC_VERIFY( gfx_api );

    auto sig_key = ResSignatureKey{ &p_shader, p_set };
    auto hash    = ResSignatureKeyHasher()( sig_key ); // TODO: find a way to cache this somehow

    RID sig_rid;
    if (auto it = res_signature_cache.find( sig_key ); it != res_signature_cache.end()) {
        sig_rid = it->second;
        NC_LOG_DEBUG_C(
            log::GRAPHICS, "ResourceSignature_{} cache HIT (rid={}) -> Shader='{}' BindingSet={}", hash, sig_rid.value,
            p_shader.filepath, p_set
        );
    } else {
        gfx::ResourceSignatureDesc signature;
        signature.name    = "ResourceSignature_" + std::to_string( hash );
        signature.set_idx = p_set;

        NC_LOG_DEBUG_C(
            log::GRAPHICS, "{} cache MISS -> creating new for shader='{}' BindingSet={}", signature.name,
            p_shader.filepath, p_set
        );

        auto params = p_shader.get_param_set( p_set );
        for (auto& param : params) {
            if (!is_bindable_resource( param.resource_type ))
                continue;

            gfx::PipelineResourceDesc res;
            res.name          = param.name;
            res.resource_type = param.resource_type;
            res.stage         = param.stage_mask;
            res.flags         = param.flags;
            res.array_size    = 1;
            signature.resources.push_back( std::move( res ) );
        }

        sig_rid = gfx_api->resource_signature_create( signature );
        res_signature_cache.emplace( sig_key, sig_rid );
    }

    RID binding_rid     = gfx_api->resource_binding_create( sig_rid );
    RID res_mapping_rid = gfx_api->resource_mapping_create( p_resources );
    gfx_api->resource_binding_update( binding_rid, res_mapping_rid, p_shader.get_stage_flags() );

    auto rid = resource_sets.acquire();
    auto set = resource_sets.get_or_fail( rid );

    set->signature = sig_rid;
    set->mapping   = res_mapping_rid;
    set->binding   = binding_rid;
    set->set       = p_set;
    for (auto& r : p_resources)
        set->slot_kinds.push_back( r.kind );

    NC_LOG_DEBUG_C(
        log::GRAPHICS, "NEW resource_set -> RID={} SigRID={} BindingRID={} MappingRID={} MappedEntries={}", rid.value,
        sig_rid.value, binding_rid.value, res_mapping_rid.value, p_resources.size()
    );

    return rid;
}

RID RenderStorage::resource_set_get_signature( RID p_resource_set )
{
    return resource_sets.get_or_fail( p_resource_set )->signature;
}

void RenderStorage::resource_set_bind( RID p_resource_set )
{
    NC_VERIFY( gfx_api );
    auto* set = resource_sets.get_or_fail( p_resource_set );
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
        // h = combine( h, std::hash<std::string>{}( e.hlsl_semantic ? e.hlsl_semantic : "" ) );
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

void RenderStorage::scene_data_update( const SceneData& p_scene_data )
{
    NC_VERIFY( gfx_api );
    ensure_scene_data_ubo_();
    gfx_api->buffer_data_write( scene_data_ubo, &p_scene_data, sizeof( p_scene_data ) );
    // the SRB is committed in material_bind(), after SetPipelineState — committing it
    // here would be discarded, because binding a PSO clears the bound SRB slots
}

// ---------------------------------------------------------------------------

RID RenderStorage::material_create( const Ref<MaterialShader>& p_shader )
{
    NC_VERIFY( gfx_api );

    if (!p_shader) {
        NC_LOG_ERROR_C( log::GRAPHICS, "material_create: null MaterialShader, material not created" );
        return RID();
    }

    RID handle  = materials.acquire();
    auto mat    = materials.get_or_fail( handle );
    mat->shader = p_shader;

    ensure_scene_data_ubo_();
    ensure_light_env_();
    ensure_push_constant_signatures_();

    for (const auto& param : p_shader->get_params()) {
        if (param.binding_space != BINDING_SET_MATERIAL)
            continue;
        if (param.resource_type == gfx::ResourceType::TEXTURE_SRV ||
            param.resource_type == gfx::ResourceType::TEXTURE_UAV) {
            mat->texture_slots[param.name] = { .type = param.param_type, .texture_rid = RID() };
            mat->texture_slot_names.push_back( param.name );
        }
    }

    uint32_t param_buffer_size = 0;
    for (const auto& param : p_shader->get_params()) {
        if (param.binding_space != BINDING_SET_MATERIAL)
            continue;
        if (param.resource_type == gfx::ResourceType::CONSTANT_BUFFER) {
            auto end = static_cast<uint32_t>( param.offset + param.total_size_bytes );
            if (end > param_buffer_size)
                param_buffer_size = end;
        }
    }

    if (param_buffer_size > 0) {
        gfx::BufferDesc buf_desc;
        buf_desc.debug_name      = "MaterialParam_" + std::to_string( handle.value );
        buf_desc.size            = param_buffer_size;
        buf_desc.usage           = gfx::ResourceUsage::DYNAMIC;
        buf_desc.cpu_access_mask = gfx::ResourceCpuAccessFlags::WRITE;
        buf_desc.bind_mask       = gfx::ResourceBindFlags::UNIFORM_BUFFER;
        mat->param_buffer        = gfx_api->buffer_create( buf_desc );
        mat->param_staging.resize( param_buffer_size, 0 );
        // a DYNAMIC buffer is only mapped on the first write — Diligent asserts if the buffer
        // reaches a draw before that, so upload the zeroed staging on the first material_bind
        mat->param_dirty = true;
    }

    DynamicArray<gfx::ResourceMappingEntry> mappings;

    if (mat->param_buffer) {
        mappings.push_back(
            gfx::ResourceMappingEntry{
                .variable_name = "g_Material",
                .kind          = gfx::ResourceType::CONSTANT_BUFFER,
                .resource      = mat->param_buffer,
            }
        );
    }

    for (const auto& param : p_shader->get_params()) {
        if (param.binding_space != BINDING_SET_MATERIAL)
            continue;
        RID slot_resource = 0;
        if (param.resource_type == gfx::ResourceType::TEXTURE_SRV ||
            param.resource_type == gfx::ResourceType::TEXTURE_UAV) {
            // the descriptor set has to be complete at PSO/pipeline bind time;
            // a white placeholder keeps it valid until the real texture arrives
            slot_resource = get_fallback_texture_( param.param_type );
        } else if (param.resource_type == gfx::ResourceType::SAMPLER) {
            slot_resource = ensure_default_sampler_();
        } else {
            continue;
        }
        mappings.push_back(
            gfx::ResourceMappingEntry{
                .variable_name = param.name.c_str(),
                .kind          = param.resource_type,
                .resource      = slot_resource,
            }
        );
    }

    const bool has_material_set = p_shader->get_param_set( BINDING_SET_MATERIAL ).size() > 0;
    if (has_material_set)
        mat->material_res_set = resource_set_create( *p_shader, BINDING_SET_MATERIAL, mappings );

    bool has_push_constants = false;
    for (const auto& param : p_shader->get_params()) {
        if (static_cast<uint32_t>( param.flags ) & static_cast<uint32_t>( gfx::ResourceFlags::INLINE_CONSTANTS )) {
            has_push_constants = true;
            break;
        }
    }
    mat->has_push_constants = has_push_constants;

    // set 0 also holds parameters that slang reported but the SPIR-V dropped (dead
    // g_Material blocks etc.) — only g_LightEnv is a real set-0 descriptor
    bool has_light_env = false;
    for (const auto& param : p_shader->get_param_set( BINDING_SET_SHADER_GLOBAL )) {
        if (param.name == "g_LightEnv") {
            has_light_env = true;
            break;
        }
    }
    mat->has_light_env = has_light_env;

    auto key = get_pso_key_( p_shader );

    key.res_signatures.push_back( resource_set_get_signature( scene_data_ubo_res_set ) );
    if (has_light_env)
        key.res_signatures.push_back( resource_set_get_signature( light_env_res_set ) );
    if (has_material_set)
        key.res_signatures.push_back( resource_set_get_signature( mat->material_res_set ) );
    if (has_push_constants) {
        key.res_signatures.push_back(
            p_shader->material_type == MaterialShaderType::Spatial ? spatial_push_sig : canvas_push_sig
        );
    }

    mat->pso     = get_render_pipeline_or_create( key );
    mat->pso_key = key;

    // flat permutation: identical except it declares no depth attachment — and a pass without
    // one cannot test or write depth either
    auto flat  = key;
    flat.flags = static_cast<PSOFlags>(
        ( flat.flags & ~static_cast<PSOFlags>( PSO_DST_FMT_MASK ) & ~( PSO_DEPTH_TEST | PSO_DEPTH_WRITE ) ) |
        ( static_cast<uint64_t>( gfx::TextureFormat::UNKNOWN ) << PSO_DST_FMT_SHIFT )
    );
    mat->pso_flat     = get_render_pipeline_or_create( flat );
    mat->pso_key_flat = flat;

    NC_LOG_DEBUG_C(
        log::GRAPHICS,
        "material_create: RID={} shader='{}' textures={} param_buf={} signatures={} set0={} set1={} push={}",
        handle.value, p_shader->filepath, mat->texture_slots.size(), param_buffer_size, key.res_signatures.size(),
        p_shader->get_param_set( BINDING_SET_SHADER_GLOBAL ).size(),
        p_shader->get_param_set( BINDING_SET_MATERIAL ).size(), has_push_constants
    );

    return handle;
}

void RenderStorage::material_set_param( RID p_material, const String& p_param, const void* p_data, size_t p_data_size )
{
    NC_VERIFY( gfx_api );

    auto mat = materials.get_or_fail( p_material );
    if (!mat->param_buffer)
        return;

    const auto write_at = [&]( size_t offset ) {
        if (offset + p_data_size <= mat->param_staging.size()) {
            std::memcpy( mat->param_staging.data() + offset, p_data, p_data_size );
            mat->param_dirty = true;
            NC_LOG_DEBUG_C(
                log::GRAPHICS, "material_set_param: material={} param='{}' offset={} size={}", p_material.value,
                p_param, offset, p_data_size
            );
        }
    };

    for (const auto& desc : mat->shader->get_params()) {
        if (desc.binding_space != BINDING_SET_MATERIAL)
            continue;

        // whole-block set by top-level param name (e.g. "g_Material").
        if (desc.name == p_param) {
            write_at( desc.offset );
            return;
        }

        // match the full dotted name ("g_Material.foobar")
        // or just the leaf name ("foobar").
        for (const auto& field : desc.fields) {
            const auto dot = field.name.rfind( '.' );
            const bool leaf_match =
                ( dot != String::npos ) && ( StringView( field.name ).substr( dot + 1 ) == p_param );
            if (field.name == p_param || leaf_match) {
                write_at( field.offset );
                return;
            }
        }
    }
}

void RenderStorage::material_set_texture( RID p_material, RID p_texture, uint32_t p_slot )
{
    NC_VERIFY( gfx_api );

    auto mat = materials.get_or_fail( p_material );
    if (p_slot >= mat->texture_slot_names.size())
        return;

    const auto& slot_name = mat->texture_slot_names[p_slot];
    auto it               = mat->texture_slots.find( slot_name );
    if (it == mat->texture_slots.end())
        return;

    if (!p_texture || !gfx_api->is_rid_owned( p_texture )) {
        p_texture = get_fallback_texture_( it->second.type );
    }

    it->second.texture_rid = p_texture;

    // update the resource mapping for this texture in the material's resource set
    auto set = resource_sets.get_or_fail( mat->material_res_set );
    gfx_api->resource_mapping_add_entry(
        set->mapping,
        gfx::ResourceMappingEntry{
            .variable_name = slot_name.c_str(),
            .kind          = gfx::ResourceType::TEXTURE_SRV,
            .resource      = p_texture,
        },
        false
    );
    gfx_api->resource_binding_update( set->binding, set->mapping, mat->shader->get_stage_flags() );

    NC_LOG_TRACE_C(
        log::GRAPHICS, "material_set_texture: material={} slot={} texture={}", p_material.value, p_slot, p_texture.value
    );
}

void RenderStorage::material_set_draw_mode( RID p_material, gfx::FillMode p_mode )
{
    NC_VERIFY( gfx_api );
    auto mat  = materials.get_or_fail( p_material );
    auto& key = mat->pso_key;
    key.flags =
        static_cast<PSOFlags>( ( key.flags & ~PSO_FILL_MASK ) | ( static_cast<uint64_t>( p_mode ) << PSO_FILL_SHIFT ) );
    mat->pso = get_render_pipeline_or_create( key );

    auto& flat = mat->pso_key_flat;
    flat.flags = static_cast<PSOFlags>(
        ( flat.flags & ~PSO_FILL_MASK ) | ( static_cast<uint64_t>( p_mode ) << PSO_FILL_SHIFT )
    );
    mat->pso_flat = get_render_pipeline_or_create( flat );
}

bool RenderStorage::material_has_push_constants( RID p_material )
{
    auto mat = materials.get( p_material );
    if (!mat)
        return false;
    return mat->has_push_constants;
}

RID RenderStorage::material_create_texture_variant( RID p_material, RID p_texture )
{
    auto mat = materials.get_or_fail( p_material );

    TextureVariantKey key{ .material = p_material, .texture = p_texture };
    if (auto it = texture_variant_cache.find( key ); it != texture_variant_cache.end())
        return it->second;

    // Clone the material and bake the override into slot 0. The base material is never
    // mutated, so its texture slots stay stable across frames regardless of draw order.
    RID variant         = material_create( mat->shader );
    auto vmat           = materials.get_or_fail( variant );
    vmat->param_staging = mat->param_staging;
    vmat->param_dirty   = mat->param_dirty || !mat->param_staging.empty();
    material_set_texture( variant, p_texture, 0 );

    texture_variant_cache.insert( { key, variant } );
    return variant;
}

void RenderStorage::material_bind( RID handle, bool p_pass_has_depth )
{
    NC_VERIFY( gfx_api );
    auto mat = materials.get_or_fail( handle );

    // Diligent discards dynamic allocations at the end of every frame — a buffer that is
    // drawn to has to be re-mapped in every frame, even when its contents did not change.
    if (mat->param_buffer && ( mat->param_dirty || mat->param_frame != frame_index )) {
        gfx_api->buffer_data_write( mat->param_buffer, mat->param_staging.data(), mat->param_staging.size() );
        mat->param_dirty = false;
        mat->param_frame = frame_index;
    }

    const RID pso = p_pass_has_depth ? mat->pso : mat->pso_flat;
    gfx_api->render_pipeline_bind( pso );
    // every signature of the PSO needs its SRB committed after the PSO is bound —
    // Diligent clears the SRB slots on SetPipelineState (index == resource set id)
    resource_set_bind( scene_data_ubo_res_set );
    if (mat->material_res_set)
        resource_set_bind( mat->material_res_set );
    if (light_env_res_set && mat->has_light_env) {
        if (light_env_frame != frame_index) {
            gfx_api->buffer_data_write( light_env_ubo, light_env_data_.data(), light_env_data_.size() );
            light_env_frame = frame_index;
        }
        resource_set_bind( light_env_res_set );
    }

    NC_LOG_TRACE_C( log::GRAPHICS, "material_bind: PSO rid={} depth_pass={}", pso.value, p_pass_has_depth );
}

// ---------------------------------------------------------------------------

RID RenderStorage::mesh_create( const Mesh& mesh )
{
    NC_VERIFY( gfx_api );

    auto rid    = meshes.acquire();
    auto result = meshes.get_or_fail( rid );

    NC_LOG_DEBUG_C(
        log::GRAPHICS, "mesh_create: RID={} vert_count={} idx_count={}", rid.value, mesh.vertex_count(),
        mesh.index_count()
    );

    gfx::BufferDesc vdesc;
    auto basename      = std::format( "{}_{}", mesh.get_class_name(), rid.value );
    vdesc.debug_name   = basename + "_VBO";
    vdesc.size         = mesh.get_vertices().size();
    vdesc.initial_data = mesh.get_vertices().data();
    vdesc.usage        = gfx::ResourceUsage::IMMUTABLE;
    vdesc.bind_mask    = gfx::ResourceBindFlags::VERTEX_BUFFER;
    result->vertices   = gfx_api->buffer_create( vdesc );

    gfx::BufferDesc idesc;
    idesc.debug_name    = basename + "_IBO";
    idesc.size          = mesh.get_indices().size_bytes();
    idesc.initial_data  = mesh.get_indices().data();
    idesc.usage         = gfx::ResourceUsage::IMMUTABLE;
    idesc.bind_mask     = gfx::ResourceBindFlags::INDEX_BUFFER;
    result->indices     = gfx_api->buffer_create( idesc );
    result->index_count = static_cast<uint32_t>( mesh.index_count() );

    return rid;
}

void RenderStorage::mesh_bind( RID handle )
{
    NC_VERIFY( gfx_api );
    auto mesh = meshes.get_or_fail( handle );
    gfx_api->buffer_vertices_bind( { &mesh->vertices, 1 }, 0 );
    gfx_api->buffer_index_bind( mesh->indices, 0 );
}

uint32_t RenderStorage::mesh_get_index_count( RID p_gpu_mesh )
{
    return meshes.get_or_fail( p_gpu_mesh )->index_count;
}

// ---------------------------------------------------------------------------

bool RenderStorage::is_rid_owned( RID rid )
{
    return resource_sets.contains( rid ) || materials.contains( rid ) || meshes.contains( rid );
}

bool RenderStorage::destroy_rid( RID rid )
{
    if (is_rid_owned( rid )) {
        pending_destroys.insert( rid );
        return true;
    }
    return false;
}

void RenderStorage::flush_pending_destroys()
{
    for (auto& rid : pending_destroys) {
        NC_LOG_DEBUG_C( log::GRAPHICS, "Flushing pending destroys: RID={}", rid.value );
        if (resource_sets.release( rid ))
            continue;
        if (materials.release( rid ))
            continue;
        if (meshes.release( rid ))
            continue;
    }
    pending_destroys.clear();
}

void RenderStorage::set_graphics_api( GfxInterface* p_gfx_api )
{
    gfx_api = p_gfx_api;
}

// ---------------------------------------------------------------------------

RenderStorage::PSOKey RenderStorage::get_pso_key_( const Ref<MaterialShader>& p_shader )
{
    PSOKey key;
    if (!p_shader) {
        NC_LOG_ERROR_C( log::GRAPHICS, "get_pso_key_: null MaterialShader" );
        return key;
    }
    key.debug_name = p_shader->filepath;

    auto ms = p_shader->multisample_state;

    key.flags = static_cast<PSOFlags>(
        ( static_cast<uint64_t>( gfx::TextureFormat::RGBA8_UNORM_SRGB ) << PSO_RT_FMT_SHIFT ) |
        ( static_cast<uint64_t>( p_shader->cull_mode ) << PSO_CULL_SHIFT ) |
        ( static_cast<uint64_t>( p_shader->fill_mode ) << PSO_FILL_SHIFT ) |
        ( p_shader->depth_test ? PSO_DEPTH_TEST : 0 ) | ( p_shader->depth_write ? PSO_DEPTH_WRITE : 0 ) |
        ( static_cast<uint64_t>( p_shader->blend ) << PSO_BLEND_SHIFT ) |
        ( static_cast<uint64_t>( ms.count & 0xF ) << PSO_MSAA_COUNT_SHIFT ) |
        ( static_cast<uint64_t>( ms.quality & 0xF ) << PSO_MSAA_QUALITY_SHIFT ) | PSO_SCISSOR
    );
    // The main pass always binds a D32 DSV and Vulkan's dynamic-rendering rules require
    // the pipeline's declared depthAttachmentFormat to match it — including for pipelines
    // that never test or write depth (canvas), which would otherwise declare UNDEFINED.
    key.flags = static_cast<PSOFlags>(
        key.flags | static_cast<uint64_t>( gfx::TextureFormat::D32_FLOAT ) << PSO_DST_FMT_SHIFT
    );

    if (auto vs_desc = p_shader->get_stage_desc( gfx::ShaderStage::VERTEX )) {
        key.vs = gfx_api->shader_create(
            gfx::ShaderCreateDesc{
                .name = "VertexShader_" + key.debug_name, .stage = vs_desc->stage, .bytecode = vs_desc->bytecode
            }
        );
    }
    if (auto ps_desc = p_shader->get_stage_desc( gfx::ShaderStage::PIXEL )) {
        key.ps = gfx_api->shader_create(
            gfx::ShaderCreateDesc{
                .name = "PixelShader_" + key.debug_name, .stage = ps_desc->stage, .bytecode = ps_desc->bytecode
            }
        );
    }

    if (p_shader->material_type == MaterialShaderType::Spatial) {
        key.vertex_layout = get_vertex3d_layout();
    } else {
        key.vertex_layout = get_vertex2d_layout();
    }

    // key.vertex_layout = p_shader->has_stage( gfx::ShaderStage::VERTEX )
    //                           ? p_shader->get_stage_desc( gfx::ShaderStage::VERTEX )->vert_layout
    //                           : gfx::VertexLayout{};

    return key;
}

// ---------------------------------------------------------------------------

bool RenderStorage::ResSignatureKey::operator==( const ResSignatureKey& o ) const
{
    if (shader != o.shader)
        return false;
    if (set != o.set)
        return false;
    return true;
}

std::size_t RenderStorage::ResSignatureKeyHasher::operator()( const ResSignatureKey& p ) const
{
    auto combine = []( size_t a, size_t b ) -> size_t { return a ^ ( b + 0x9e3779b9 + ( a << 6 ) + ( a >> 2 ) ); };
    size_t h     = std::hash<uint16_t>{}( p.set );
    h            = combine( h, reinterpret_cast<size_t>( p.shader ) );
    return h;
}

bool RenderStorage::TextureVariantKey::operator==( const TextureVariantKey& o ) const
{
    return material == o.material && texture == o.texture;
}

std::size_t RenderStorage::TextureVariantKeyHasher::operator()( const TextureVariantKey& p ) const
{
    auto combine = []( size_t a, size_t b ) -> size_t { return a ^ ( b + 0x9e3779b9 + ( a << 6 ) + ( a >> 2 ) ); };
    size_t h     = std::hash<uint64_t>{}( p.material.value );
    h            = combine( h, std::hash<uint64_t>{}( p.texture.value ) );
    return h;
}

// ---------------------------------------------------------------------------

void RenderStorage::ensure_scene_data_ubo_()
{
    if (scene_data_ubo)
        return;

    gfx::BufferDesc sdb_desc;
    sdb_desc.debug_name      = "SceneData_UBO";
    sdb_desc.size            = sizeof( SceneData );
    sdb_desc.usage           = gfx::ResourceUsage::DYNAMIC;
    sdb_desc.cpu_access_mask = gfx::ResourceCpuAccessFlags::WRITE;
    sdb_desc.bind_mask       = gfx::ResourceBindFlags::UNIFORM_BUFFER;
    scene_data_ubo           = gfx_api->buffer_create( sdb_desc );

    gfx::ResourceSignatureDesc sdb_sig_desc;
    sdb_sig_desc.name      = "SceneData_UBO_Signature";
    sdb_sig_desc.set_idx   = RenderStorage::BINDING_SET_SCENE;
    sdb_sig_desc.resources = { gfx::PipelineResourceDesc{
        .name          = "g_Scene",
        .stage         = gfx::ShaderStage::VS_PS,
        .resource_type = gfx::ResourceType::CONSTANT_BUFFER,
        .array_size    = 1
    } };

    RID sig = gfx_api->resource_signature_create( sdb_sig_desc );

    gfx::ResourceMappingEntry mapping;
    mapping.variable_name = "g_Scene";
    mapping.kind          = gfx::ResourceType::CONSTANT_BUFFER;
    mapping.resource      = scene_data_ubo;
    RID res_mapping       = gfx_api->resource_mapping_create( { &mapping, 1 } );

    RID binding = gfx_api->resource_binding_create( sig );
    gfx_api->resource_binding_update( binding, res_mapping, gfx::ShaderStage::VS_PS );

    auto set_rid   = resource_sets.acquire();
    auto set       = resource_sets.get_or_fail( set_rid );
    set->signature = sig;
    set->mapping   = res_mapping;
    set->binding   = binding;
    set->set       = BINDING_SET_SCENE;
    set->slot_kinds.push_back( gfx::ResourceType::CONSTANT_BUFFER );

    scene_data_ubo_res_set = set_rid;

    NC_LOG_DEBUG_C(
        log::GRAPHICS, "ensure_scene_data_ubo_: created SceneData_UBO size={} RID={} ResSet={}", sizeof( SceneData ),
        scene_data_ubo.value, set_rid.value
    );
}

// ---------------------------------------------------------------------------

void RenderStorage::ensure_light_env_()
{
    NC_VERIFY( gfx_api );

    if (light_env_ubo)
        return;

    // Sized generously: the shader block is `LightArray<DirectionalLight, 4>`.
    gfx::BufferDesc buf_desc;
    buf_desc.debug_name      = "LightEnv_UBO";
    buf_desc.size            = 512;
    buf_desc.usage           = gfx::ResourceUsage::DYNAMIC;
    buf_desc.cpu_access_mask = gfx::ResourceCpuAccessFlags::WRITE;
    buf_desc.bind_mask       = gfx::ResourceBindFlags::UNIFORM_BUFFER;
    light_env_ubo            = gfx_api->buffer_create( buf_desc );

    // The block must contain at least one light: `illuminate()` walks
    // `lights[0..count)` and a zero-filled block means `count == 0`, i.e. every
    // spatial surface renders pure black. Until a real light system exists, a
    // single default sun keeps the scene lit.
    //
    // Layout mirrors the shader (std140 and HLSL constant-buffer packing agree):
    //   LightArray           offset  0: int  count
    //   DirectionalLight[4]  offset 16: stride 32 per element
    //     float3 direction     element + 0   (points from the surface toward the light)
    //     float3 intensity     element + 16
    light_env_data_.assign( buf_desc.size, 0 );
    auto put_u32 = [&]( size_t off, uint32_t v ) { std::memcpy( light_env_data_.data() + off, &v, sizeof( v ) ); };
    auto put_f32 = [&]( size_t off, float v ) { std::memcpy( light_env_data_.data() + off, &v, sizeof( v ) ); };

    constexpr size_t kCountOffset     = 0;
    constexpr size_t kLightBase       = 16;
    constexpr size_t kDirOffset       = 0;
    constexpr size_t kIntensityOffset = 16;

    put_u32( kCountOffset, 1 );
    put_f32( kLightBase + kDirOffset + 0, 0.0f ); // 30 degrees above the horizon
    put_f32( kLightBase + kDirOffset + 4, 0.5f );
    put_f32( kLightBase + kDirOffset + 8, 0.8660254f );
    put_f32( kLightBase + kIntensityOffset + 0, 3.0f );
    put_f32( kLightBase + kIntensityOffset + 4, 3.0f );
    put_f32( kLightBase + kIntensityOffset + 8, 3.0f );

    // Written again by material_bind() on first use: Diligent discards dynamic
    // buffer allocations at frame boundaries, so a write made here may not survive.
    gfx_api->buffer_data_write( light_env_ubo, light_env_data_.data(), light_env_data_.size() );

    gfx::ResourceSignatureDesc sig_desc;
    sig_desc.name      = "LightEnv_Signature";
    sig_desc.set_idx   = RenderStorage::BINDING_SET_SHADER_GLOBAL;
    sig_desc.resources = { gfx::PipelineResourceDesc{
        .name          = "g_LightEnv",
        .stage         = gfx::ShaderStage::PIXEL,
        .resource_type = gfx::ResourceType::CONSTANT_BUFFER,
        .array_size    = 1
    } };

    RID sig = gfx_api->resource_signature_create( sig_desc );

    gfx::ResourceMappingEntry mapping;
    mapping.variable_name = "g_LightEnv";
    mapping.kind          = gfx::ResourceType::CONSTANT_BUFFER;
    mapping.resource      = light_env_ubo;
    RID res_mapping       = gfx_api->resource_mapping_create( { &mapping, 1 } );

    RID binding = gfx_api->resource_binding_create( sig );
    gfx_api->resource_binding_update( binding, res_mapping, gfx::ShaderStage::PIXEL );

    auto set_rid   = resource_sets.acquire();
    auto set       = resource_sets.get_or_fail( set_rid );
    set->signature = sig;
    set->mapping   = res_mapping;
    set->binding   = binding;
    set->set       = BINDING_SET_SHADER_GLOBAL;
    set->slot_kinds.push_back( gfx::ResourceType::CONSTANT_BUFFER );

    light_env_res_set = set_rid;

    NC_LOG_DEBUG_C(
        log::GRAPHICS, "ensure_light_env_: created LightEnv size=512 RID={} ResSet={}", light_env_ubo.value,
        set_rid.value
    );
}

// ---------------------------------------------------------------------------

void RenderStorage::ensure_push_constant_signatures_()
{
    NC_VERIFY( gfx_api );

    if (!spatial_push_sig) {
        gfx::ResourceSignatureDesc sig_desc;
        sig_desc.name        = "SpatialPushConstants";
        sig_desc.set_idx     = BINDING_SET_INSTANCE;
        sig_desc.resources   = { gfx::PipelineResourceDesc{
            .name          = "g_Instance",
            .stage         = gfx::ShaderStage::VERTEX,
            .resource_type = gfx::ResourceType::CONSTANT_BUFFER,
            .flags         = gfx::ResourceFlags::INLINE_CONSTANTS,
            .array_size    = SPATIAL_PUSH_CONSTANT_UINTS
        } };
        spatial_push_sig     = gfx_api->resource_signature_create( sig_desc );
        spatial_push_binding = gfx_api->resource_binding_create( spatial_push_sig );
    }

    if (!canvas_push_sig) {
        gfx::ResourceSignatureDesc sig_desc;
        sig_desc.name       = "CanvasPushConstants";
        sig_desc.set_idx    = BINDING_SET_INSTANCE;
        sig_desc.resources  = { gfx::PipelineResourceDesc{
            .name          = "g_Instance",
            .stage         = gfx::ShaderStage::VERTEX,
            .resource_type = gfx::ResourceType::CONSTANT_BUFFER,
            .flags         = gfx::ResourceFlags::INLINE_CONSTANTS,
            .array_size    = CANVAS_PUSH_CONSTANT_UINTS
        } };
        canvas_push_sig     = gfx_api->resource_signature_create( sig_desc );
        canvas_push_binding = gfx_api->resource_binding_create( canvas_push_sig );
    }
}

// ---------------------------------------------------------------------------

void RenderStorage::ensure_fallback_textures_()
{
    NC_VERIFY( gfx_api );

    auto make = [&]( const char* name, gfx::ResourceDimension dim, uint32_t layers ) {
        uint8_t pixels[4] = { 255, 255, 255, 255 };
        gfx::TextureDesc desc{};
        desc.debug_name      = name;
        desc.format          = gfx::TextureFormat::RGBA8_UNORM_SRGB;
        desc.dimension       = dim;
        desc.usage           = gfx::ResourceUsage::DYNAMIC;
        desc.cpu_access_mask = gfx::ResourceCpuAccessFlags::WRITE;
        desc.width           = 1;
        desc.height          = 1;
        desc.array_size      = layers;
        for (uint32_t i = 0; i < layers; ++i) {
            desc.subresources.emplace_back( pixels );
        }
        return gfx_api->texture_create( desc );
    };

    if (!gfx_api->is_rid_owned( white_texture_2d ))
        white_texture_2d = make( "WhiteTexture2D", gfx::ResourceDimension::DIM_2D, 1 );
    if (!gfx_api->is_rid_owned( white_texture_2d_array ))
        white_texture_2d_array = make( "WhiteTexture2DArray", gfx::ResourceDimension::DIM_2D_ARRAY, 1 );
    if (!gfx_api->is_rid_owned( white_texture_cube ))
        white_texture_cube = make( "WhiteTextureCube", gfx::ResourceDimension::DIM_CUBE, 6 );
}

RID RenderStorage::ensure_default_sampler_()
{
    NC_VERIFY( gfx_api );

    if (default_sampler)
        return default_sampler;

    gfx::SamplerDesc desc;
    desc.debug_name = "DefaultSampler";
    default_sampler = gfx_api->sampler_create( desc );
    return default_sampler;
}

RID RenderStorage::get_fallback_texture_( gfx::ParameterType type )
{
    ensure_fallback_textures_();
    if (type == gfx::ParameterType::UNKNOWN)
        return white_texture_2d;
    if (any( type & gfx::ParameterType::TEXTURE_2D_ARRAY ))
        return white_texture_2d_array;
    if (any( type & gfx::ParameterType::TEXTURE_CUBED ))
        return white_texture_cube;
    return white_texture_2d;
}

// ---------------------------------------------------------------------------

uint32_t RenderStorage::align_up_( uint32_t p_value, uint32_t p_alignment )
{
    // bitwise trick for rounding upwards to the multiple of `p_alignment`
    return ( p_value + p_alignment - 1 ) & ~( p_alignment - 1 );
}

} // namespace nc
