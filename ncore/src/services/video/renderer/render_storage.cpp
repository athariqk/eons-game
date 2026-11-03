#include <algorithm>
#include <cstring>
#include <format>

#include <ncore/services/video/gfx_interface.h>
#include <ncore/services/video/renderer/render_storage.h>
#include <ncore/services/video/renderer/vertex_format.h>
#include <ncore/utils/assert.h>
#include <ncore/utils/log.h>
#include <utils/profiler.h>

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
        NC_LOG_TRACE_C(
            log::GRAPHICS, "CACHE HIT render PSO '{}_{}' (rid={})", key.debug_name, hash, it->second.value
        );
        return it->second;
    }

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

    NC_LOG_DEBUG_C( log::GRAPHICS, "NEW render PSO RID={} -> cache MISS ('{}_{}')", rid.value, key.debug_name, hash );

    return rid;
}

RID RenderStorage::get_compute_pipeline_or_create( const PSOKey& key )
{
    auto hash = PSOKeyHasher()( key ); // TODO: find a way to cache this somehow

    auto it = pso_cache.find( key );
    if (it != pso_cache.end()) {
        NC_LOG_TRACE_C(
            log::GRAPHICS, "CACHE HIT compute PSO '{}_{}' (rid={})", key.debug_name, hash, it->second.value
        );
        return it->second;
    }

    NC_VERIFY( gfx_api );

    gfx::ComputePSODesc desc{};
    desc.debug_name          = key.debug_name;
    desc.compute_shader      = key.cs;
    desc.resource_signatures = key.res_signatures;

    auto rid = gfx_api->compute_pipeline_create( desc );
    pso_cache.emplace( key, rid );

    NC_LOG_DEBUG_C( log::GRAPHICS, "NEW compute PSO RID={} -> cache MISS ('{}_{}')", rid.value, key.debug_name, hash );

    return rid;
}

RID RenderStorage::compute_pipeline_create( RID p_shader, Span<const RID> p_resource_sets )
{
    auto* program = shader_programs.get( p_shader );
    if (!program || !program->cs) {
        NC_LOG_ERROR_C(
            log::GRAPHICS, "compute_pipeline_create: RID {} is not a compute shader program", p_shader.value
        );
        return RID();
    }

    PSOKey key;
    key.debug_name = "ComputePSO_" + program->debug_name;
    key.cs         = program->cs;
    for (auto& set : p_resource_sets)
        key.res_signatures.push_back( resource_set_get_signature( set ) );

    return get_compute_pipeline_or_create( key );
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
    return false;
}

// ---------------------------------------------------------------------------

RID RenderStorage::shader_program_create( const ShaderProgramDesc& p_desc )
{
    NC_ZONE_COLOR( "RenderStorage::shader_program_create", NC_COLOR_RENDER )

    NC_VERIFY( gfx_api );

    if (p_desc.modules.empty()) {
        NC_LOG_ERROR_C( log::GRAPHICS, "shader_program_create: no stages for '{}'", p_desc.debug_name );
        return RID();
    }

    RID handle    = shader_programs.acquire();
    auto* program = shader_programs.get_or_fail( handle );

    program->debug_name  = String( p_desc.debug_name );
    program->render_type = p_desc.render_type;
    program->params.assign( p_desc.params.begin(), p_desc.params.end() );

    for (size_t idx = 0; idx < p_desc.params.size(); ++idx) {
        if (p_desc.params[idx].binding_space != RenderStorage::BINDING_SET_MATERIAL)
            continue;
        program->material_param_to_index[p_desc.params[idx].base_name] = idx;
    }

    for (const auto& module : p_desc.modules) {
        program->stage_mask = program->stage_mask | module.stage;

        if (program->vertex_layout.size() == 0 && module.vert_layout.size() > 0)
            program->vertex_layout = module.vert_layout;

        RID shader_rid = gfx_api->shader_create(
            gfx::ShaderCreateDesc{
                .name     = std::format( "{}_{}", p_desc.debug_name, TypeDB::get_enum_name( &module.stage ) ),
                .stage    = module.stage,
                .bytecode = module.bytecode,
            }
        );

        if (!shader_rid.is_valid()) {
            NC_LOG_ERROR_C(
                log::GRAPHICS, "shader_program_create: stage {} of '{}' failed", TypeDB::get_enum_name( &module.stage ),
                p_desc.debug_name
            );
            continue;
        }

        if (any( module.stage & gfx::ShaderStage::VERTEX ))
            program->vs = shader_rid;
        else if (any( module.stage & gfx::ShaderStage::PIXEL ))
            program->ps = shader_rid;
        else if (any( module.stage & gfx::ShaderStage::COMPUTE ))
            program->cs = shader_rid;
    }

    NC_LOG_DEBUG_C(
        log::GRAPHICS, "NEW shader program RID={} name='{}' stages={} params={} vs={} ps={} cs={}", handle.value,
        program->debug_name, program->params.size(), program->params.size(), program->vs.value, program->ps.value,
        program->cs.value
    );

    return handle;
}

RID RenderStorage::resource_set_create(
    RID p_shader, gfx::BindingSet p_set, Span<const gfx::ResourceMappingEntry> p_resources
)
{
    NC_VERIFY( gfx_api );

    auto* program = shader_programs.get( p_shader );
    if (!program) {
        NC_LOG_ERROR_C( log::GRAPHICS, "resource_set_create: RID {} is not a shader program", p_shader.value );
        return RID();
    }

    auto sig_key = ResSignatureKey{ p_shader, p_set };
    auto hash    = ResSignatureKeyHasher()( sig_key ); // TODO: find a way to cache this somehow

    RID sig_rid;
    if (auto it = res_signature_cache.find( sig_key ); it != res_signature_cache.end()) {
        sig_rid = it->second;
        NC_LOG_TRACE_C(
            log::GRAPHICS, "CACHE HIT ResourceSignature_{} (rid={}) -> Shader='{}' BindingSet={}", hash, sig_rid.value,
            program->debug_name, p_set
        );
    } else {
        gfx::ResourceSignatureDesc signature;
        signature.name    = "ResourceSignature_" + std::to_string( hash );
        signature.set_idx = p_set;

        for (const auto& param : program->params) {
            if (param.binding_space != p_set)
                continue;
            if (!is_bindable_resource( param.resource_type ))
                continue;

            gfx::PipelineResourceDesc res;
            res.name          = param.qualified_name;
            res.resource_type = param.resource_type;
            res.stage         = param.stage_mask;
            res.flags         = param.flags;
            res.array_size    = 1;
            signature.resources.push_back( std::move( res ) );
        }

        sig_rid = gfx_api->resource_signature_create( signature );

        NC_LOG_DEBUG_C(
            log::GRAPHICS, "NEW resource signature RID={} name={} shader='{}' BindingSet={} -> cache MISS",
            sig_rid.value, signature.name, program->debug_name, p_set
        );

        res_signature_cache.emplace( sig_key, sig_rid );
    }

    RID binding_rid     = gfx_api->resource_binding_create( sig_rid );
    RID res_mapping_rid = gfx_api->resource_mapping_create( p_resources );
    gfx_api->resource_binding_update( binding_rid, res_mapping_rid, program->stage_mask );

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
    NC_ZONE_COLOR( "RenderStorage::resource_set_bind", NC_COLOR_RENDER )
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
    NC_ZONE_COLOR( "RenderStorage::scene_data_update", NC_COLOR_RENDER )
    NC_VERIFY( gfx_api );
    ensure_global_uniforms();

    // DEFAULT buffer persists across frames: skip the upload when nothing changed
    // (identical multi-pass frames and repeated calls).
    if (is_scene_data_valid && std::memcmp( &last_scene_data, &p_scene_data, sizeof( SceneData ) ) == 0)
        return;
    last_scene_data     = p_scene_data;
    is_scene_data_valid = true;

    gfx_api->buffer_data_write( scene_data_ubo, &p_scene_data, sizeof( p_scene_data ) );

    // the globals SRB is committed in material_bind().
}

// ---------------------------------------------------------------------------

RID RenderStorage::material_create( const MaterialCreateDesc& p_desc )
{
    NC_ZONE_COLOR( "RenderStorage::material_create", NC_COLOR_RENDER )

    NC_VERIFY( gfx_api );

    auto* program = shader_programs.get( p_desc.shader );
    if (!program) {
        NC_LOG_ERROR_C(
            log::GRAPHICS, "FAIL creating material: shader program RID {} is not owned by the renderer",
            p_desc.shader.value
        );
        return RID();
    }

    RID handle = materials.acquire();
    auto* mat  = materials.get_or_fail( handle );

    mat->shader      = p_desc.shader;
    mat->render_type = p_desc.render_type;
    mat->state       = p_desc.state;
    mat->transparent = p_desc.state.blend != gfx::BlendPreset::OPAQUE;

    ensure_global_uniforms();
    ensure_push_constant_signatures();

    uint32_t param_buffer_size = p_desc.param_buffer_size > 0 ? align_up( p_desc.param_buffer_size, 256 ) : 0;
    mat->param_buffer_size     = param_buffer_size;

    if (param_buffer_size > 0) {
        gfx::BufferDesc buf_desc;
        buf_desc.debug_name = "MaterialParam_" + std::to_string( handle.value );
        buf_desc.size       = param_buffer_size;
        buf_desc.usage      = gfx::ResourceUsage::DEFAULT;
        buf_desc.bind_mask  = gfx::ResourceBindFlags::UNIFORM_BUFFER;
        mat->param_buffer   = gfx_api->buffer_create( buf_desc );
        mat->param_staging.assign( param_buffer_size, 0 );
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

    bool has_material_set = false;
    for (const auto& param : program->params) {
        if (param.binding_space != BINDING_SET_MATERIAL)
            continue;
        has_material_set  = true;
        RID slot_resource = 0;
        if (param.resource_type == gfx::ResourceType::TEXTURE_SRV ||
            param.resource_type == gfx::ResourceType::TEXTURE_UAV) {
            // the descriptor set has to be complete at PSO/pipeline bind time;
            // start with the white fallback and let material_set_texture() replace it.
            slot_resource = get_fallback_texture( param.param_type );
        } else if (param.resource_type == gfx::ResourceType::SAMPLER) {
            slot_resource = ensure_default_sampler();
        } else if (param.resource_type == gfx::ResourceType::CONSTANT_BUFFER) {
            continue; // already mapped as g_Material
        } else {
            continue;
        }

        mappings.push_back(
            gfx::ResourceMappingEntry{
                .variable_name = param.qualified_name.c_str(),
                .kind          = param.resource_type,
                .resource      = slot_resource,
            }
        );
    }

    if (has_material_set)
        mat->material_res_set = resource_set_create( p_desc.shader, BINDING_SET_MATERIAL, mappings );

    bool has_push_constants = false;
    for (const auto& param : program->params) {
        if (static_cast<uint32_t>( param.flags ) & static_cast<uint32_t>( gfx::ResourceFlags::INLINE_CONSTANTS )) {
            has_push_constants = true;
            break;
        }
    }
    mat->has_push_constants = has_push_constants;

    bool has_light_env = false;
    bool has_scene     = false;
    for (const auto& param : program->params) {
        if (param.binding_space != BINDING_SET_SHADER_GLOBAL)
            continue;
        if (param.qualified_name == LIGHTING_CBUFFER_NAME)
            has_light_env = true;
        if (param.qualified_name == SCENE_CBUFFER_NAME)
            has_scene = true;
    }
    mat->has_globals = has_light_env || has_scene;

    auto key = get_pso_key( p_desc.shader, p_desc.state );

    if (mat->has_globals)
        key.res_signatures.push_back( resource_set_get_signature( global_uniforms_res_set ) );
    if (has_material_set)
        key.res_signatures.push_back( resource_set_get_signature( mat->material_res_set ) );
    if (has_push_constants) {
        key.res_signatures.push_back( p_desc.render_type == RenderType::SPATIAL ? spatial_push_sig : canvas_push_sig );
    }

    mat->pso     = get_render_pipeline_or_create( key );
    mat->pso_key = key;

    // Same permutation, no depth attachment (DSVFormat = UNKNOWN).
    auto key_flat     = key;
    key_flat.flags    = static_cast<PSOFlags>( key.flags & ~static_cast<uint64_t>( PSO_DST_FMT_MASK ) );
    mat->pso_flat     = get_render_pipeline_or_create( key_flat );
    mat->pso_key_flat = key_flat;

    NC_LOG_DEBUG_C(
        log::GRAPHICS, "NEW material RID={} PSO={} shader='{}' param_buf={} signatures={} set0={} set1={} push={}",
        handle.value, mat->pso.value, program->debug_name, param_buffer_size, key.res_signatures.size(),
        program->params.size(), mappings.size(), has_push_constants
    );

    return handle;
}

void RenderStorage::material_set_data( RID p_material, uint32_t p_offset, const void* p_data, size_t p_data_size )
{
    NC_ZONE_COLOR( "RenderStorage::material_set_data", NC_COLOR_RENDER )

    NC_VERIFY( gfx_api );

    auto* mat = materials.get_or_fail( p_material );
    if (!mat->param_buffer)
        return;

    if (!p_data || p_data_size == 0)
        return;

    if (static_cast<size_t>( p_offset ) + p_data_size > mat->param_staging.size()) {
        NC_LOG_WARN_C(
            log::GRAPHICS, "material_set_data: offset={} size={} overflows material={} block size={}", p_offset,
            p_data_size, p_material.value, mat->param_staging.size()
        );
        return;
    }

    std::memcpy( mat->param_staging.data() + p_offset, p_data, p_data_size );
    mat->param_dirty = true;
}

void RenderStorage::material_set_texture( RID p_material, StringView p_param_name, RID p_texture )
{
    NC_ZONE_COLOR( "RenderStorage::material_set_texture", NC_COLOR_RENDER )

    NC_VERIFY( gfx_api );

    auto* mat = materials.get_or_fail( p_material );
    if (!mat->material_res_set)
        return;

    auto* program = shader_programs.get( mat->shader );
    if (!program)
        return;

    const auto param_it = program->material_param_to_index.find( p_param_name );
    const ShaderParamDesc* param =
        ( param_it != program->material_param_to_index.end() ) ? &program->params[param_it->second] : nullptr;
    if (!param || ( param->resource_type != gfx::ResourceType::TEXTURE_SRV &&
                    param->resource_type != gfx::ResourceType::TEXTURE_UAV )) {
        NC_LOG_WARN_C(
            log::GRAPHICS, "material_set_texture: material={} has no texture parameter named '{}'", p_material.value,
            p_param_name
        );
        return;
    }

    // if (auto it = mat->texture_bindings.find( String( p_param_name ) );
    //     it != mat->texture_bindings.end() && it->second == p_texture)
    //     return;

    if (!p_texture || !gfx_api->is_rid_owned( p_texture )) {
        p_texture = get_fallback_texture( param->param_type );
        NC_LOG_WARN_C(
            log::GRAPHICS, "material_set_texture: material={} param='{}' texture RID {} is invalid, using fallback {}",
            p_material.value, p_param_name, p_texture.value, p_texture.value
        );
    }

    // mat->texture_bindings[String( p_param_name )] = p_texture;

    // update the resource mapping for this texture in the material's resource set
    auto* set = resource_sets.get_or_fail( mat->material_res_set );
    gfx_api->resource_mapping_add_entry(
        set->mapping,
        gfx::ResourceMappingEntry{
            .variable_name = param->qualified_name.c_str(),
            .kind          = param->resource_type,
            .resource      = p_texture,
        },
        false
    );

    gfx_api->resource_binding_update(
        set->binding, set->mapping, program ? program->stage_mask : gfx::ShaderStage::VS_PS
    );

    // invalidate the bound material cache
    if (p_material == last_bound_material)
        is_bound_valid = false;

    NC_LOG_TRACE_C(
        log::GRAPHICS, "material_set_texture: material={} param='{}' texture={}", p_material.value, p_param_name,
        p_texture.value
    );
}

void RenderStorage::material_set_draw_mode( RID p_material, gfx::FillMode p_mode )
{
    NC_VERIFY( gfx_api );
    auto* mat = materials.get_or_fail( p_material );

    mat->state.fill_mode = p_mode;

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

bool RenderStorage::material_is_transparent( RID p_material )
{
    auto* mat = materials.get( p_material );
    if (!mat)
        return false;
    return mat->transparent;
}

bool RenderStorage::material_bind( RID p_handle, bool p_pass_has_depth )
{
    NC_ZONE_COLOR( "RenderStorage::material_bind", NC_COLOR_RENDER )

    NC_VERIFY( gfx_api );
    auto* mat = materials.get_or_fail( p_handle );
    // TODO: handle missing materials elegantly, e.g. bind a fallback material instead of asserting.

    if (mat->param_buffer && mat->param_dirty) {
        gfx_api->buffer_data_write( mat->param_buffer, mat->param_staging.data(), mat->param_staging.size() );
        mat->param_dirty = false;
    }

    const RID pso = p_pass_has_depth ? mat->pso : mat->pso_flat;

    // skip binding pso if the same material and same permutation are already bound.
    if (is_bound_valid && p_handle == last_bound_material && pso == last_bound_pso) {
        NC_LOG_TRACE_C(
            log::GRAPHICS, "material_bind: cache HIT, PSO rid={} depth_pass={}", pso.value, p_pass_has_depth
        );
        return false;
    }

    gfx_api->render_pipeline_bind( pso );

    // every signature of the PSO needs its SRB committed after the PSO is bound
    if (mat->has_globals)
        resource_set_bind( global_uniforms_res_set );
    if (mat->material_res_set)
        resource_set_bind( mat->material_res_set );

    last_bound_material = p_handle;
    last_bound_pso      = pso;
    is_bound_valid      = true;

    NC_LOG_TRACE_C( log::GRAPHICS, "material_bind: PSO rid={} depth_pass={}", pso.value, p_pass_has_depth );
    return true;
}

// ---------------------------------------------------------------------------

RID RenderStorage::mesh_create( const MeshUploadDesc& p_desc )
{
    NC_ZONE_COLOR( "RenderStorage::mesh_create", NC_COLOR_RENDER )

    NC_VERIFY( gfx_api );

    if (p_desc.vertices.empty() || p_desc.indices.empty() || p_desc.vertex_stride == 0) {
        NC_LOG_ERROR_C( log::GRAPHICS, "mesh_create: empty vertex/index data" );
        return RID();
    }

    auto rid     = meshes.acquire();
    auto* result = meshes.get_or_fail( rid );

    NC_LOG_DEBUG_C(
        log::GRAPHICS, "NEW mesh RID={} vert_count={} idx_count={} layers={}", rid.value,
        p_desc.vertices.size() / p_desc.vertex_stride, p_desc.indices.size(), p_desc.layers.size()
    );

    gfx::BufferDesc vdesc;
    auto basename      = std::format( "Mesh_{}", rid.value );
    vdesc.debug_name   = basename + "_VBO";
    vdesc.size         = p_desc.vertices.size();
    vdesc.initial_data = p_desc.vertices.data();
    vdesc.usage        = gfx::ResourceUsage::IMMUTABLE;
    vdesc.bind_mask    = gfx::ResourceBindFlags::VERTEX_BUFFER;
    result->vertices   = gfx_api->buffer_create( vdesc );

    gfx::BufferDesc idesc;
    idesc.debug_name    = basename + "_IBO";
    idesc.size          = p_desc.indices.size() * sizeof( uint32_t );
    idesc.initial_data  = p_desc.indices.data();
    idesc.usage         = gfx::ResourceUsage::IMMUTABLE;
    idesc.bind_mask     = gfx::ResourceBindFlags::INDEX_BUFFER;
    result->indices     = gfx_api->buffer_create( idesc );
    result->index_count = static_cast<uint32_t>( p_desc.indices.size() );

    // CPU-side copy of the layer table.
    if (p_desc.layers.empty()) {
        result->layer_count           = 1;
        result->layers[0]             = {};
        result->layers[0].first_index = 0;
        result->layers[0].index_count = result->index_count;
        result->layers[0].base_vertex = 0;
    } else {
        NC_ASSERT_MSG(
            p_desc.layers.size() <= GPUMesh::MAX_LAYERS, "Mesh has more layers than MAX_LAYERS (split the mesh)"
        );
        result->layer_count = static_cast<uint8_t>( p_desc.layers.size() );
        std::copy( p_desc.layers.begin(), p_desc.layers.end(), result->layers.begin() );
    }

    return rid;
}

void RenderStorage::mesh_bind( RID handle )
{
    NC_ZONE_COLOR( "RenderStorage::mesh_bind", NC_COLOR_RENDER )
    NC_VERIFY( gfx_api );
    auto* mesh = meshes.get_or_fail( handle );
    gfx_api->buffer_vertices_bind( { &mesh->vertices, 1 }, 0 );
    gfx_api->buffer_index_bind( mesh->indices, 0 );
}

uint32_t RenderStorage::mesh_get_index_count( RID p_gpu_mesh )
{
    return meshes.get_or_fail( p_gpu_mesh )->index_count;
}

uint8_t RenderStorage::mesh_get_layer_count( RID p_gpu_mesh )
{
    return meshes.get_or_fail( p_gpu_mesh )->layer_count;
}

bool RenderStorage::mesh_get_layer( RID p_gpu_mesh, uint32_t p_index, MeshLayerDesc& p_out )
{
    auto* mesh = meshes.get_or_fail( p_gpu_mesh );
    if (p_index >= mesh->layer_count)
        return false;
    p_out = mesh->layers[p_index];
    return true;
}

// ---------------------------------------------------------------------------

bool RenderStorage::is_rid_owned( RID rid )
{
    return resource_sets.contains( rid ) || materials.contains( rid ) || meshes.contains( rid ) ||
           shader_programs.contains( rid );
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
    NC_ZONE_COLOR( "RenderStorage::flush_pending_destroys", NC_COLOR_RENDER )

    for (auto& rid : pending_destroys) {
        NC_LOG_DEBUG_C( log::GRAPHICS, "Flushing pending destroys: RID={}", rid.value );
        if (resource_sets.contains( rid )) {
            release_resource_set( rid );
            continue;
        }
        if (auto* program = shader_programs.get( rid )) {
            release_shader_program( *program );
            shader_programs.release( rid );
            continue;
        }
        if (auto* mat = materials.get( rid )) {
            release_material_gpu( *mat );
            materials.release( rid );
            if (last_bound_material == rid)
                is_bound_valid = false;
            continue;
        }
        if (auto* mesh = meshes.get( rid )) {
            gfx_api->destroy_rid( mesh->vertices );
            gfx_api->destroy_rid( mesh->indices );
            meshes.release( rid );
            continue;
        }
    }
    pending_destroys.clear();
}

void RenderStorage::release_material_gpu( MaterialData& p_material )
{
    if (p_material.param_buffer) {
        gfx_api->destroy_rid( p_material.param_buffer );
        p_material.param_buffer = 0;
    }
    if (p_material.material_res_set) {
        release_resource_set( p_material.material_res_set );
        p_material.material_res_set = 0;
    }
    // p_material.texture_bindings.clear();
}

void RenderStorage::release_shader_program( ShaderProgramData& p_program )
{
    if (p_program.vs) {
        gfx_api->destroy_rid( p_program.vs );
        p_program.vs = 0;
    }
    if (p_program.ps) {
        gfx_api->destroy_rid( p_program.ps );
        p_program.ps = 0;
    }
    if (p_program.cs) {
        gfx_api->destroy_rid( p_program.cs );
        p_program.cs = 0;
    }
}

void RenderStorage::release_resource_set( RID p_resource_set )
{
    auto* rs = resource_sets.get( p_resource_set );
    if (!rs)
        return;
    if (rs->mapping)
        gfx_api->destroy_rid( rs->mapping );
    if (rs->binding)
        gfx_api->destroy_rid( rs->binding );
    resource_sets.release( p_resource_set );
}

void RenderStorage::set_graphics_api( GfxInterface* p_gfx_api )
{
    if (gfx_api == p_gfx_api)
        return;

    gfx_api = p_gfx_api;
    ++device_revision;
}

void RenderStorage::shutdown()
{
    flush_pending_destroys();

    const bool clean = resource_sets.get_size() == 0 && shader_programs.get_size() == 0 && materials.get_size() == 0 &&
                       meshes.get_size() == 0;
    if (!clean) {
        NC_LOG_WARN_C(
            log::GRAPHICS,
            "RenderStorage::shutdown: pools not empty (resource_sets={} shader_programs={} materials={} meshes={}) — "
            "resources outlived the device",
            resource_sets.get_size(), shader_programs.get_size(), materials.get_size(), meshes.get_size()
        );
    }

    // Every remaining handle died with the device; drop the bookkeeping.
    resource_sets.release_all();
    shader_programs.release_all();
    materials.release_all();
    meshes.release_all();
    pso_cache.clear();
    res_signature_cache.clear();
    pending_destroys.clear();

    is_bound_valid          = false;
    last_bound_material     = RID::INVALID;
    last_bound_pso          = RID::INVALID;
    is_scene_data_valid     = false;
    global_uniforms_res_set = RID::INVALID;
    scene_data_ubo          = RID::INVALID;
    light_env_ubo           = RID::INVALID;
    spatial_push_sig        = RID::INVALID;
    spatial_push_binding    = RID::INVALID;
    canvas_push_sig         = RID::INVALID;
    canvas_push_binding     = RID::INVALID;
    white_texture_2d        = RID::INVALID;
    white_texture_2d_array  = RID::INVALID;
    white_texture_cube      = RID::INVALID;
    default_sampler         = RID::INVALID;

    // The device this storage talked to is gone.
    gfx_api = nullptr;
    ++device_revision;
}

// ---------------------------------------------------------------------------

RenderStorage::PSOKey RenderStorage::get_pso_key( RID p_shader, const MaterialStateDesc& p_state )
{
    PSOKey key;

    auto* program = shader_programs.get( p_shader );
    if (!program) {
        NC_LOG_ERROR_C( log::GRAPHICS, "get_pso_key: RID {} is not a shader program", p_shader.value );
        return key;
    }

    key.debug_name = program->debug_name;
    key.vs         = program->vs;
    key.ps         = program->ps;

    auto ms = p_state.multisample;

    key.flags = static_cast<PSOFlags>(
        ( static_cast<uint64_t>( gfx::TextureFormat::RGBA8_UNORM_SRGB ) << PSO_RT_FMT_SHIFT ) |
        ( static_cast<uint64_t>( p_state.cull_mode ) << PSO_CULL_SHIFT ) |
        ( static_cast<uint64_t>( p_state.fill_mode ) << PSO_FILL_SHIFT ) | ( p_state.depth_test ? PSO_DEPTH_TEST : 0 ) |
        ( p_state.depth_write ? PSO_DEPTH_WRITE : 0 ) | ( static_cast<uint64_t>( p_state.blend ) << PSO_BLEND_SHIFT ) |
        ( static_cast<uint64_t>( ms.count & 0xF ) << PSO_MSAA_COUNT_SHIFT ) |
        ( static_cast<uint64_t>( ms.quality & 0xF ) << PSO_MSAA_QUALITY_SHIFT ) | PSO_SCISSOR
    );
    // The main pass always binds a D32 DSV and Vulkan's dynamic-rendering rules require
    // the pipeline's declared depthAttachmentFormat to match it — including for pipelines
    // that never test or write depth (canvas), which would otherwise declare UNDEFINED.
    key.flags = static_cast<PSOFlags>(
        key.flags | static_cast<uint64_t>( gfx::TextureFormat::D32_FLOAT ) << PSO_DST_FMT_SHIFT
    );

    key.vertex_layout = program->render_type == RenderType::SPATIAL ? get_vertex3d_layout() : get_vertex2d_layout();

    return key;
}

// ---------------------------------------------------------------------------

bool RenderStorage::ResSignatureKey::operator==( const ResSignatureKey& o ) const
{
    if (shader_rid != o.shader_rid)
        return false;
    if (set != o.set)
        return false;
    return true;
}

std::size_t RenderStorage::ResSignatureKeyHasher::operator()( const ResSignatureKey& p ) const
{
    auto combine = []( size_t a, size_t b ) -> size_t { return a ^ ( b + 0x9e3779b9 + ( a << 6 ) + ( a >> 2 ) ); };
    size_t h     = std::hash<uint16_t>{}( p.set );
    h            = combine( h, std::hash<uint64_t>{}( p.shader_rid.value ) );
    return h;
}

// ---------------------------------------------------------------------------

void RenderStorage::ensure_global_uniforms()
{
    NC_VERIFY( gfx_api );

    if (global_uniforms_res_set)
        return;

    // --- SceneData UBO (g_Scene) ---

    gfx::BufferDesc sdb_desc;
    sdb_desc.debug_name = "SceneData_UBO";
    sdb_desc.size       = sizeof( SceneData );
    sdb_desc.usage      = gfx::ResourceUsage::DEFAULT;
    sdb_desc.bind_mask  = gfx::ResourceBindFlags::UNIFORM_BUFFER;
    scene_data_ubo      = gfx_api->buffer_create( sdb_desc );

    // --- LightEnv UBO (g_LightEnv) ---

    // Sized generously: the shader block is `LightArray<DirectionalLight, 4>`.
    gfx::BufferDesc buf_desc;
    buf_desc.debug_name = "LightEnv_UBO";
    buf_desc.size       = 512;
    buf_desc.usage      = gfx::ResourceUsage::DEFAULT;
    buf_desc.bind_mask  = gfx::ResourceBindFlags::UNIFORM_BUFFER;
    light_env_ubo       = gfx_api->buffer_create( buf_desc );

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
    light_env_data.assign( buf_desc.size, 0 );
    auto put_u32 = [&]( size_t off, uint32_t v ) { std::memcpy( light_env_data.data() + off, &v, sizeof( v ) ); };
    auto put_f32 = [&]( size_t off, float v ) { std::memcpy( light_env_data.data() + off, &v, sizeof( v ) ); };

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

    gfx_api->buffer_data_write( light_env_ubo, light_env_data.data(), light_env_data.size() );

    // --- one combined set-0 signature/SRB for both blocks ---
    // Set 0 (BINDING_SET_SHADER_GLOBAL) holds g_LightEnv at binding 1 and
    // g_Scene at binding 0 (see scene.slang). Sharing a single signature means
    // one SRB and therefore one CommitShaderResources per PSO switch instead of
    // one per uniform block.
    gfx::ResourceSignatureDesc sig_desc;
    sig_desc.name      = "GlobalUniforms_Signature";
    sig_desc.set_idx   = RenderStorage::BINDING_SET_SHADER_GLOBAL;
    sig_desc.resources = {
        gfx::PipelineResourceDesc{
            .name          = "g_Scene",
            .stage         = gfx::ShaderStage::VS_PS,
            .resource_type = gfx::ResourceType::CONSTANT_BUFFER,
            .array_size    = 1
        },
        gfx::PipelineResourceDesc{
            .name          = "g_LightEnv",
            .stage         = gfx::ShaderStage::PIXEL,
            .resource_type = gfx::ResourceType::CONSTANT_BUFFER,
            .array_size    = 1
        }
    };

    RID sig = gfx_api->resource_signature_create( sig_desc );

    gfx::ResourceMappingEntry mappings[2] = {
        gfx::ResourceMappingEntry{
            .variable_name = "g_Scene",
            .kind          = gfx::ResourceType::CONSTANT_BUFFER,
            .resource      = scene_data_ubo,
        },
        gfx::ResourceMappingEntry{
            .variable_name = "g_LightEnv",
            .kind          = gfx::ResourceType::CONSTANT_BUFFER,
            .resource      = light_env_ubo,
        }
    };
    RID res_mapping = gfx_api->resource_mapping_create( mappings );

    RID binding = gfx_api->resource_binding_create( sig );
    gfx_api->resource_binding_update( binding, res_mapping, gfx::ShaderStage::VS_PS );

    auto set_rid   = resource_sets.acquire();
    auto* set      = resource_sets.get_or_fail( set_rid );
    set->signature = sig;
    set->mapping   = res_mapping;
    set->binding   = binding;
    set->set       = BINDING_SET_SHADER_GLOBAL;
    set->slot_kinds.push_back( gfx::ResourceType::CONSTANT_BUFFER );
    set->slot_kinds.push_back( gfx::ResourceType::CONSTANT_BUFFER );

    global_uniforms_res_set = set_rid;

    NC_LOG_DEBUG_C(
        log::GRAPHICS, "ensure_global_uniforms: created SceneData size={} RID={} LightEnv size=512 RID={} ResSet={}",
        sizeof( SceneData ), scene_data_ubo.value, light_env_ubo.value, set_rid.value
    );
}

// ---------------------------------------------------------------------------

void RenderStorage::ensure_push_constant_signatures()
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

void RenderStorage::ensure_fallback_textures()
{
    NC_VERIFY( gfx_api );

    auto make = [&]( const char* name, gfx::ResourceDimension dim, uint32_t layers ) {
        uint8_t pixels[4] = { 255, 255, 255, 255 };
        gfx::TextureDesc desc{};
        desc.debug_name      = name;
        desc.format          = gfx::TextureFormat::RGBA8_UNORM_SRGB;
        desc.dimension       = dim;
        desc.usage           = gfx::ResourceUsage::IMMUTABLE;
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

RID RenderStorage::ensure_default_sampler()
{
    NC_VERIFY( gfx_api );

    if (default_sampler)
        return default_sampler;

    gfx::SamplerDesc desc;
    desc.debug_name = "DefaultSampler";
    default_sampler = gfx_api->sampler_create( desc );
    return default_sampler;
}

RID RenderStorage::get_fallback_texture( gfx::ParameterType type )
{
    ensure_fallback_textures();
    if (type == gfx::ParameterType::UNKNOWN)
        return white_texture_2d;
    if (any( type & gfx::ParameterType::TEXTURE_2D_ARRAY ))
        return white_texture_2d_array;
    if (any( type & gfx::ParameterType::TEXTURE_CUBED ))
        return white_texture_cube;
    return white_texture_2d;
}

// ---------------------------------------------------------------------------

uint32_t RenderStorage::align_up( uint32_t p_value, uint32_t p_alignment )
{
    // bitwise trick for rounding upwards to the multiple of `p_alignment`
    return ( p_value + p_alignment - 1 ) & ~( p_alignment - 1 );
}

} // namespace nc
