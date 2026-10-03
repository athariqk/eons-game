#include "slang_module_compiler.h"

#include <filesystem>
#include <sstream>
#include <vector>

#include <ncore/resources/resource.h>
#include <ncore/resources/resource_archive.h>
#include <ncore/services/video/gfx_types.h>
#include <ncore/utils/log.h>

namespace nc {

SlangModuleCompiler::SlangModuleCompiler()
{
    SlangResult r = slang::createGlobalSession( global_session.writeRef() );
    if (SLANG_FAILED( r )) {
        NC_LOG_ERROR_C( log::GRAPHICS, "SlangModuleCompiler: failed to create Slang global session" );
    } else {
        NC_LOG_TRACE_C( log::GRAPHICS, "SlangModuleCompiler initialized" );
    }
}

SlangModuleCompiler::~SlangModuleCompiler()
{
    slang::shutdown();
}

StringView SlangModuleCompiler::get_format_name() const
{
    return "Shader";
}

bool SlangModuleCompiler::is_handling_extension( const String& ext )
{
    return ext == ".slang";
}

static gfx::ParameterType translate_slang_type( slang::TypeReflection* type )
{
    if (!type)
        return gfx::ParameterType::UNKNOWN;

    // unwrap array types to get the base element type
    while (type && type->getKind() == slang::TypeReflection::Kind::Array) {
        type = type->getElementType();
    }
    if (!type)
        return gfx::ParameterType::UNKNOWN;

    auto kind   = type->getKind();
    auto scalar = type->getScalarType();

    // textures and samplers
    if (kind == slang::TypeReflection::Kind::Resource) {
        // NOTE: previously masked the shape with SLANG_RESOURCE_BASE_SHAPE_MASK
        // *before* testing the array flag, which lives outside that mask — this
        // meant is_array could never be true. Test the raw shape first.
        auto raw_shape = type->getResourceShape();
        auto is_array  = ( raw_shape & SLANG_TEXTURE_ARRAY_FLAG ) != 0;
        auto shape     = raw_shape & SLANG_RESOURCE_BASE_SHAPE_MASK;
        switch (shape) {
            case SLANG_TEXTURE_2D:
                return is_array ? gfx::ParameterType::TEXTURE_2D_ARRAY : gfx::ParameterType::TEXTURE_2D;
            case SLANG_TEXTURE_CUBE:
                return is_array ? gfx::ParameterType::TEXTURE_CUBED_ARRAY : gfx::ParameterType::TEXTURE_CUBED;
            default:
                break;
        }
    }

    if (kind == slang::TypeReflection::Kind::SamplerState) {
        return gfx::ParameterType::SAMPLER;
    }

    // matrices
    if (kind == slang::TypeReflection::Kind::Matrix) {
        size_t row_count = type->getRowCount();
        size_t col_count = type->getColumnCount();

        if (scalar == slang::TypeReflection::ScalarType::Float32 && row_count == 4 && col_count == 4)
            return gfx::ParameterType::MAT4;

        NC_LOG_WARN_C(
            log::GRAPHICS, "translate_slang_type: unsupported matrix shape {}x{}, scalar={}", row_count, col_count,
            static_cast<int>( scalar )
        );
        return gfx::ParameterType::UNKNOWN;
    }

    // vectors
    if (kind == slang::TypeReflection::Kind::Vector) {
        size_t elem_count = type->getElementCount();
        switch (scalar) {
            case slang::TypeReflection::ScalarType::Float32:
                switch (elem_count) {
                    case 2:
                        return gfx::ParameterType::FLOAT2;
                    case 3:
                        return gfx::ParameterType::FLOAT3;
                    case 4:
                        return gfx::ParameterType::FLOAT4;
                    default:
                        break;
                }
                break;

            case slang::TypeReflection::ScalarType::Int32:
                switch (elem_count) {
                    case 2:
                        return gfx::ParameterType::INT2;
                    case 3:
                        return gfx::ParameterType::INT3;
                    case 4:
                        return gfx::ParameterType::INT4;
                    default:
                        break;
                }
                break;

            case slang::TypeReflection::ScalarType::UInt16:
                if (elem_count == 4)
                    return gfx::ParameterType::USHORT4;
                break;

            case slang::TypeReflection::ScalarType::UInt8:
                if (elem_count == 4)
                    return gfx::ParameterType::UBYTE4_NORM;
                break;

            default:
                break;
        }

        NC_LOG_WARN_C(
            log::GRAPHICS, "translate_slang_type: unsupported vector element count {}, scalar={}", elem_count,
            static_cast<int>( scalar )
        );
        return gfx::ParameterType::UNKNOWN;
    }

    // pure scalars
    if (kind == slang::TypeReflection::Kind::Scalar) {
        switch (scalar) {
            case slang::TypeReflection::ScalarType::Float16:
                return gfx::ParameterType::FLOAT_16;
            case slang::TypeReflection::ScalarType::Float32:
                return gfx::ParameterType::FLOAT_32; // Or gfx::ParameterType::FLOAT
            case slang::TypeReflection::ScalarType::Float64:
                return gfx::ParameterType::FLOAT_64;
            case slang::TypeReflection::ScalarType::Int8:
                return gfx::ParameterType::INT_8;
            case slang::TypeReflection::ScalarType::Int16:
                return gfx::ParameterType::INT_16;
            case slang::TypeReflection::ScalarType::Int32:
                return gfx::ParameterType::INT_32; // Or gfx::ParameterType::INT
            case slang::TypeReflection::ScalarType::Int64:
                return gfx::ParameterType::INT_64;
            case slang::TypeReflection::ScalarType::Bool:
                return gfx::ParameterType::BOOL;
            case slang::TypeReflection::ScalarType::UInt32:
                return gfx::ParameterType::UINT;
            case slang::TypeReflection::ScalarType::UInt16:
                return gfx::ParameterType::USHORT;
            default:
                break;
        }
    }

    NC_LOG_WARN_C(
        log::GRAPHICS, "translate_slang_type: unhandled Slang TypeReflection Kind={}, Scalar={}",
        static_cast<int>( kind ), static_cast<int>( scalar )
    );
    return gfx::ParameterType::UNKNOWN;
}

static gfx::ResourceType classify_resource_type( slang::TypeReflection* type )
{
    auto shape      = type->getResourceShape();
    auto base_shape = shape & SLANG_RESOURCE_BASE_SHAPE_MASK;

    switch (base_shape) {
        case SLANG_STRUCTURED_BUFFER:
            if (type->getResourceAccess() == SLANG_RESOURCE_ACCESS_READ_WRITE) {
                return gfx::ResourceType::BUFFER_UAV;
            }
            [[fallthrough]]; // fall to readonly buffer type
        case SLANG_BYTE_ADDRESS_BUFFER:
            return gfx::ResourceType::BUFFER_SRV;
        case SLANG_TEXTURE_1D:
        case SLANG_TEXTURE_2D:
        case SLANG_TEXTURE_3D:
        case SLANG_TEXTURE_CUBE:
        case SLANG_TEXTURE_BUFFER:
            if (type->getResourceAccess() == SLANG_RESOURCE_ACCESS_READ_WRITE) {
                return gfx::ResourceType::TEXTURE_UAV;
            }
            return gfx::ResourceType::TEXTURE_SRV;
        default:
            break;
    }
    NC_LOG_WARN_C(
        log::GRAPHICS, "classify_resource_type: unhandled resource base shape {}", static_cast<int>( base_shape )
    );
    return gfx::ResourceType::UNKNOWN;
}

static String get_slang_parameter_name( slang::ParameterCategory category )
{
    switch (category) {
        case slang::ParameterCategory::None:
            return "None";
        case slang::ParameterCategory::Mixed:
            return "Mixed";
        case slang::ParameterCategory::ConstantBuffer: // Same target as MetalBuffer
            return "Constant Buffer";
        case slang::ParameterCategory::ShaderResource: // Same target as MetalTexture
            return "Shader Resource";
        case slang::ParameterCategory::UnorderedAccess:
            return "Unordered Access";
        case slang::ParameterCategory::VaryingInput:  // Same target as VertexInput
            return "Varying Input";
        case slang::ParameterCategory::VaryingOutput: // Same target as FragmentOutput
            return "Varying Output";
        case slang::ParameterCategory::SamplerState:
            return "Sampler State";
        case slang::ParameterCategory::Uniform:
            return "Uniform";
        case slang::ParameterCategory::DescriptorTableSlot:
            return "Descriptor Table Slot";
        case slang::ParameterCategory::SpecializationConstant:
            return "Specialization Constant";
        case slang::ParameterCategory::PushConstantBuffer:
            return "Push Constant Buffer";
        case slang::ParameterCategory::RegisterSpace:
            return "Register Space";
        case slang::ParameterCategory::GenericResource:
            return "Generic Resource";
        case slang::ParameterCategory::RayPayload:
            return "Ray Payload";
        case slang::ParameterCategory::HitAttributes:
            return "Hit Attributes";
        case slang::ParameterCategory::CallablePayload:
            return "Callable Payload";
        case slang::ParameterCategory::ShaderRecord:
            return "Shader Record";
        case slang::ParameterCategory::ExistentialTypeParam:
            return "Existential Type Param";
        case slang::ParameterCategory::ExistentialObjectParam:
            return "Existential Object Param";
        case slang::ParameterCategory::SubElementRegisterSpace:
            return "Sub Element Register Space";
        case slang::ParameterCategory::InputAttachmentIndex:
            return "Input Attachment Index";
        case slang::ParameterCategory::MetalArgumentBufferElement:
            return "Metal Argument Buffer Element";
        case slang::ParameterCategory::MetalAttribute:
            return "Metal Attribute";
        case slang::ParameterCategory::MetalPayload:
            return "Metal Payload";
    }
    return "Unknown";
}

Error SlangModuleCompiler::compile( StringView p_input, StringView p_output, AssetImportContext ctx )
{
    if (p_input.empty()) {
        NC_LOG_ERROR_C( log::IO, "Filepath to modules are required for compilation" );
        return Error::ERR_INVALID_PARAMETER;
    }

    set_search_paths_( ctx.search_paths );

    if (!ensure_session_( ctx.skip_cache ))
        return Error::FAIL;

    auto fs_path      = std::filesystem::path( p_input.data() );
    auto fullpath_str = fs_path.string();
    auto path_key     = fs_path.filename().string();

    slang::IModule* module = nullptr;
    {
        Slang::ComPtr<slang::IBlob> diags;
        module = compile_session->loadModule( fullpath_str.c_str(), diags.writeRef() );

        if (diags && diags->getBufferSize() > 0) {
            auto diags_str = static_cast<const char*>( diags->getBufferPointer() );
            NC_LOG_DEBUG_C( log::IO, "{}", diags_str );
        }

        if (!module) {
            return Error::FAIL;
        }
    }

    // See also https://docs.shader-slang.org/en/latest/compilation-api.html#get-target-kernel-code

    MaterialShaderDesc mat_shader_desc;
    reflect_material_attributes_( module, mat_shader_desc );

    std::vector<Slang::ComPtr<slang::IEntryPoint>> entry_points;

    for (SlangInt32 i = 0; i < module->getDefinedEntryPointCount(); i++) {
        Slang::ComPtr<slang::IEntryPoint> ep;
        if (SLANG_FAILED( module->getDefinedEntryPoint( i, ep.writeRef() ) )) {
            NC_LOG_ERROR_C( log::IO, "Getting entry point at idx '{}' failed", i );
            continue;
        }
        entry_points.push_back( std::move( ep ) );
    }

    if (entry_points.empty()) {
        NC_LOG_INFO_C( log::IO, "Shader '{}' defines no entry points, skipping compilation", path_key );
        return Error::OK;
    }

    DynamicArray<ShaderDesc> programs;

    for (auto& entry_point : entry_points) {
        slang::IEntryPoint* ep = entry_point.get();

        slang::IComponentType* components[] = { module, ep };
        Slang::ComPtr<slang::IComponentType> composed;
        {
            Slang::ComPtr<slang::IBlob> diags;
            auto r =
                compile_session->createCompositeComponentType( components, 2, composed.writeRef(), diags.writeRef() );
            if (diags && diags->getBufferSize() > 0)
                NC_LOG_DEBUG_C( log::IO, "{}", static_cast<const char*>( diags->getBufferPointer() ) );
            if (SLANG_FAILED( r ))
                continue;
        }
        // A generic entry point (vertexMain<M : ISpatialMaterial>) — or a module-level
        // material parameter (ICanvasMaterial g_Material) — still carries an unsatisfied
        // type parameter until it is bound to the concrete material struct.
        const auto spec_count                        = composed->getSpecializationParamCount();
        Slang::ComPtr<slang::IComponentType> to_link = composed;
        if (spec_count > 0) {
            if (!mat_shader_desc.material_type || spec_count != 1) {
                NC_LOG_ERROR_C(
                    log::IO, "Shader '{}' has {} specialization parameter(s) that cannot be bound to the material type",
                    path_key, spec_count
                );
                continue;
            }

            slang::SpecializationArg arg = slang::SpecializationArg::fromType( mat_shader_desc.material_type );

            Slang::ComPtr<slang::IComponentType> specialized;
            Slang::ComPtr<slang::IBlob> diags;
            auto r = composed->specialize( &arg, 1, specialized.writeRef(), diags.writeRef() );
            if (diags && diags->getBufferSize() > 0)
                NC_LOG_DEBUG_C( log::IO, "{}", static_cast<const char*>( diags->getBufferPointer() ) );
            if (SLANG_FAILED( r ) || !specialized) {
                NC_LOG_ERROR_C(
                    log::IO, "Specializing shader '{}' with material type '{}' failed", path_key,
                    mat_shader_desc.material_type->getName()
                );
                continue;
            }
            to_link = specialized;
        }

        Slang::ComPtr<slang::IComponentType> linked;
        {
            Slang::ComPtr<slang::IBlob> diags;
            auto r = to_link->link( linked.writeRef(), diags.writeRef() );
            if (diags && diags->getBufferSize() > 0)
                NC_LOG_DEBUG_C( log::IO, "{}", static_cast<const char*>( diags->getBufferPointer() ) );
            if (SLANG_FAILED( r ))
                continue;
        }

        Slang::ComPtr<slang::IBlob> spirv_code;
        {
            Slang::ComPtr<slang::IBlob> diags;
            auto r = linked->getEntryPointCode( 0, 0, spirv_code.writeRef(), diags.writeRef() );
            if (diags && diags->getBufferSize() > 0)
                NC_LOG_DEBUG_C( log::IO, "{}", static_cast<const char*>( diags->getBufferPointer() ) );
            if (SLANG_FAILED( r ) || !spirv_code) {
                NC_LOG_ERROR_C(
                    log::IO, "Generating target code for '{}' failed (0x{:08X})", path_key, static_cast<uint32_t>( r )
                );
                continue;
            }
        }

        auto* prog_layout = linked->getLayout();
        if (!prog_layout) {
            NC_LOG_ERROR_C( log::IO, "Failed to get program layout for '{}'", path_key );
            continue;
        }
        auto* linked_ep_layout = prog_layout->getEntryPointByIndex( 0 );
        if (!linked_ep_layout) {
            NC_LOG_ERROR_C( log::IO, "Linked component for '{}' has no entry point layout", path_key );
            continue;
        }
        const auto stage = linked_ep_layout->getStage();

        NC_LOG_INFO_C(
            log::IO, "Shader '{}' entry point '{}' compiled (stage={}, {} bytes)", path_key,
            ep->getFunctionReflection()->getName(),
            stage == SLANG_STAGE_VERTEX     ? "VS"
            : stage == SLANG_STAGE_FRAGMENT ? "PS"
            : stage == SLANG_STAGE_COMPUTE  ? "CS"
                                            : "?",
            spirv_code->getBufferSize()
        );
        ShaderDesc entry_out;
        entry_out.entrypoint = ep->getFunctionReflection()->getName();

        //-----------------------------
        // REFLECTION PHASE

        switch (stage) {
            case SLANG_STAGE_VERTEX:
                entry_out.stage = gfx::ShaderStage::VERTEX;
                break;
            case SLANG_STAGE_FRAGMENT:
                entry_out.stage = gfx::ShaderStage::PIXEL;
                break;
            case SLANG_STAGE_COMPUTE:
                entry_out.stage = gfx::ShaderStage::COMPUTE;
                break;
            default:
                entry_out.stage = gfx::ShaderStage::NONE;
                break;
        }

        auto words   = static_cast<const uint32_t*>( spirv_code->getBufferPointer() );
        size_t count = spirv_code->getBufferSize() / sizeof( uint32_t );
        entry_out.bytecode.assign( words, words + count );

        auto entry_ep_layout = linked_ep_layout;
        if (entry_out.stage == gfx::ShaderStage::COMPUTE && entry_ep_layout) {
            SlangUInt thread_group_sizes[3] = {};
            entry_ep_layout->getComputeThreadGroupSize( 3, thread_group_sizes );
            entry_out.num_threads_x = static_cast<uint32_t>( thread_group_sizes[0] );
            entry_out.num_threads_y = static_cast<uint32_t>( thread_group_sizes[1] );
            entry_out.num_threads_z = static_cast<uint32_t>( thread_group_sizes[2] );
            NC_LOG_INFO_C(
                log::IO, "Compute shader '{}' compiled - numthreads=({}, {}, {})", entry_out.entrypoint,
                entry_out.num_threads_x, entry_out.num_threads_y, entry_out.num_threads_z
            );
        }

        reflect_module_params_( prog_layout, entry_out );
        reflect_entry_point_params_( linked_ep_layout, entry_out.stage, entry_out.params, entry_out.vert_layout );

        programs.push_back( std::move( entry_out ) );
    }

    if (programs.empty()) {
        NC_LOG_ERROR_C( log::IO, "Shader '{}' produced no entry points", path_key );
        return Error::FAIL;
    }

    Ref<IResource> resource;

    if (mat_shader_desc.is_material) {
        auto ms               = Ref<MaterialShader>::create( programs );
        ms->cull_mode         = mat_shader_desc.cull_mode;
        ms->fill_mode         = mat_shader_desc.fill_mode;
        ms->depth_test        = mat_shader_desc.depth_test;
        ms->depth_write       = mat_shader_desc.depth_write;
        ms->blend             = mat_shader_desc.blend;
        ms->multisample_state = mat_shader_desc.multisample_state;

        // material_type drives get_pso_key_'s vertex layout selection. It is inferred
        // from the material interface in reflect_material_attributes_.
        ms->material_type = mat_shader_desc.material_kind;

        resource = ms;
    } else {
        resource = Ref<Shader>::create( programs );
    }

    ResourceArchive archive;
    if (auto err = archive.serialize( resource, p_output.data() ); err != Error::OK) {
        NC_LOG_ERROR_C( log::IO, "Failed to serialize shader '{}' to '{}'", p_input, p_output );
        return err;
    }

    return Error::OK;
}

// ---------------------------------------------------------------------------

bool SlangModuleCompiler::ensure_session_( bool p_recreate_session )
{
    if (session_created && !p_recreate_session)
        return true;

    if (!global_session) {
        NC_LOG_ERROR_C( log::GRAPHICS, "SlangModuleCompiler: no global session available" );
        return false;
    }

    slang::SessionDesc sd;
    slang::TargetDesc target;
    target.format              = SLANG_SPIRV;
    target.profile             = global_session->findProfile( "spirv_1_5" );
    sd.targets                 = &target;
    sd.targetCount             = 1;
    sd.defaultMatrixLayoutMode = SlangMatrixLayoutMode::SLANG_MATRIX_LAYOUT_COLUMN_MAJOR;
    sd.searchPaths             = search_path_ptrs_.data();
    sd.searchPathCount         = static_cast<SlangInt>( search_path_ptrs_.size() );

    if (SLANG_FAILED( global_session->createSession( sd, compile_session.writeRef() ) )) {
        NC_LOG_ERROR_C( log::GRAPHICS, "SlangModuleCompiler: failed to create compile session" );
        return false;
    }

    session_created = true;
    return true;
}

// ---------------------------------------------------------------------------

void SlangModuleCompiler::set_search_paths_( const DynamicArray<String>& p_paths )
{
    if (p_paths.empty() || p_paths == search_path_storage_)
        return;

    String paths;
    search_path_storage_ = p_paths;
    search_path_ptrs_.clear();
    search_path_ptrs_.reserve( search_path_storage_.size() );
    for (const auto& path : search_path_storage_) {
        search_path_ptrs_.push_back( path.c_str() );
        paths += "\n - " + path;
    }

    // The old session still references the previous pointers; ensure_session_()
    // releases it via writeRef() before creating the replacement.
    session_created = false;

    NC_LOG_INFO_C( log::IO, "Shader search paths set to: {}", paths);
}

// ---------------------------------------------------------------------------

void SlangModuleCompiler::reflect_fields_(
    slang::TypeLayoutReflection* type_layout, uint32_t base_offset, const String& name_prefix, uint32_t binding_space,
    gfx::ShaderStage stage, DynamicArray<ShaderParamField>& out_fields, DynamicArray<ShaderParamDesc>& out_params
)
{
    if (!type_layout)
        return;

    for (unsigned f = 0; f < type_layout->getFieldCount(); f++) {
        auto field = type_layout->getFieldByIndex( f );
        if (!field)
            continue;

        auto field_type = field->getTypeLayout();
        if (!field_type)
            continue;

        const char* raw_name = field->getName();
        String field_name    = name_prefix.empty() ? String( raw_name ? raw_name : "" )
                                                   : name_prefix + "." + String( raw_name ? raw_name : "" );

        auto field_kind = field_type->getKind();

        // unwrap arrays to inspect the element kind, but remember we saw one
        auto element_type_layout = field_type;
        uint32_t array_size      = 1;
        if (field_kind == slang::TypeReflection::Kind::Array) {
            array_size          = static_cast<uint32_t>( field_type->getElementCount() );
            element_type_layout = field_type->getElementTypeLayout();
            field_kind          = element_type_layout ? element_type_layout->getKind() : field_kind;
        }

        // nested uniform struct -> recurse, keep accumulating byte offset
        if (field_kind == slang::TypeReflection::Kind::Struct) {
            reflect_fields_(
                element_type_layout, base_offset + static_cast<uint32_t>( field->getOffset() ), field_name,
                binding_space, stage, out_fields, out_params
            );
            continue;
        }

        // resource/sampler nested inside a ParameterBlock -> its own descriptor
        // binding, not byte-offset uniform data
        if (field_kind == slang::TypeReflection::Kind::Resource ||
            field_kind == slang::TypeReflection::Kind::SamplerState) {
            ShaderParamDesc info;
            info.name          = field_name;
            info.binding_space = static_cast<gfx::BindingSet>( binding_space );
            info.binding_idx   = field->getBindingIndex();
            info.stage_mask    = stage;
            info.resource_type = field_kind == slang::TypeReflection::Kind::SamplerState
                                     ? gfx::ResourceType::SAMPLER
                                     : classify_resource_type( element_type_layout->getType() );
            info.param_type    = translate_slang_type( element_type_layout->getType() );
            if (field_kind != slang::TypeReflection::Kind::SamplerState) {
                auto access     = element_type_layout->getType()->getResourceAccess();
                info.cpu_access = ( access == SLANG_RESOURCE_ACCESS_READ_WRITE )
                                      ? gfx::ResourceCpuAccessFlags::READ_WRITE
                                      : gfx::ResourceCpuAccessFlags::READ;
            }
            NC_LOG_DEBUG_C(
                log::IO, "param='{}' type='{}' space={} idx={}", info.name, rtti::get_enum_name( &info.resource_type ),
                info.binding_space, info.binding_idx
            );
            out_params.push_back( std::move( info ) );
            continue;
        }

        // plain uniform data: scalar/vector/matrix, possibly arrayed
        ShaderParamField out;
        out.name       = field_name;
        out.offset     = base_offset + static_cast<uint32_t>( field->getOffset() );
        out.size       = static_cast<uint32_t>( field_type->getSize() );
        out.stride     = field_type->getStride();
        out.type       = translate_slang_type( field_type->getType() );
        out.array_size = array_size;
        out_fields.push_back( out );

        NC_LOG_DEBUG_C(
            log::IO, "  field '{}' type='{}' offset={} size={} stride={} array_size={}", out.name,
            rtti::get_enum_name( &out.type ), out.offset, out.size, out.stride, out.array_size
        );
    }
}

void SlangModuleCompiler::reflect_module_params_( slang::ProgramLayout* p_module_layout, ShaderDesc& p_module )
{
    for (unsigned j = 0; j < p_module_layout->getParameterCount(); j++) {
        auto param = p_module_layout->getParameterByIndex( j );
        if (!param)
            continue;

        auto type_layout = param->getTypeLayout();
        if (!type_layout)
            continue;

        auto original_kind = type_layout->getKind();

        if (original_kind == slang::TypeReflection::Kind::ConstantBuffer ||
            original_kind == slang::TypeReflection::Kind::ParameterBlock) {
            type_layout = type_layout->getElementTypeLayout();
        }

        NC_LOG_DEBUG_C(
            log::IO, "param='{}' category='{}' kind={} field_count={} space={} idx={}", param->getName(),
            get_slang_parameter_name( param->getCategory() ), static_cast<int>( original_kind ),
            type_layout->getFieldCount(), param->getBindingSpace(), param->getBindingIndex()
        );

        ShaderParamDesc info;
        info.name             = param->getName() ? param->getName() : ""; // avoid std::string ctor(nullptr_t) crash
        info.total_size_bytes = type_layout->getSize();
        info.stage_mask       = p_module.stage;
        info.binding_space    = static_cast<gfx::BindingSet>( param->getBindingSpace() );
        info.binding_idx      = param->getBindingIndex();

        switch (original_kind) {
            case slang::TypeReflection::Kind::ConstantBuffer:
                info.resource_type = gfx::ResourceType::CONSTANT_BUFFER;
                info.element_stride =
                    type_layout->getElementStride( SlangParameterCategory::SLANG_PARAMETER_CATEGORY_CONSTANT_BUFFER );
                if (param->getCategory() == slang::ParameterCategory::PushConstantBuffer) {
                    info.flags = gfx::ResourceFlags::INLINE_CONSTANTS;
                }
                break;
            case slang::TypeReflection::Kind::ParameterBlock:
                // ParameterBlock<T> is its own descriptor set, distinct from a
                // plain ConstantBuffer<T>. It may contain a mix of uniform data
                // and resources/samplers — those are pulled out into their own
                // ShaderParamDesc entries by reflect_fields_ below rather than
                // living in `fields`.
                info.resource_type = gfx::ResourceType::CONSTANT_BUFFER;

                // In the case of SubElementRegisterSpace category, Slang got the definition
                // of D3D12's "space" and "index" term in reverse??
                info.binding_space = static_cast<gfx::BindingSet>( param->getBindingIndex() );
                info.binding_idx   = param->getBindingSpace();
                break;
            case slang::TypeReflection::Kind::Resource:
                info.resource_type = classify_resource_type( type_layout->getType() );
                // param_type drives descriptor wiring (which fallback texture a material binds
                // for an unset slot) — without it the slot defaults to UNKNOWN, whose all-ones
                // mask makes every texture-dimension test match.
                info.param_type = translate_slang_type( type_layout->getType() );
                {
                    auto access     = type_layout->getType()->getResourceAccess();
                    info.cpu_access = ( access == SLANG_RESOURCE_ACCESS_READ_WRITE )
                                          ? gfx::ResourceCpuAccessFlags::READ_WRITE
                                          : gfx::ResourceCpuAccessFlags::READ;
                }
                break;
            case slang::TypeReflection::Kind::SamplerState:
                info.resource_type = gfx::ResourceType::SAMPLER;
                info.param_type    = gfx::ParameterType::SAMPLER;
                break;
            default:
                info.resource_type = gfx::ResourceType::CONSTANT_BUFFER;
                info.element_stride =
                    type_layout->getElementStride( SlangParameterCategory::SLANG_PARAMETER_CATEGORY_CONSTANT_BUFFER );
                break;
        }

        // Binding space/index come from Slang's own reflection. For ParameterBlock the
        // terms are reversed (see the switch above), and the concrete material block now
        // carries an explicit [[vk::binding]], so the values are authoritative without
        // consulting SPIR-V.

        NC_LOG_DEBUG_C(
            log::IO, "  binding '{}' -> set={} idx={}", info.name, static_cast<uint32_t>( info.binding_space ),
            info.binding_idx
        );

        // Recursively reflect fields for anything struct-shaped (plain struct,
        // ConstantBuffer<T>, or ParameterBlock<T>). Resource/sampler params
        // themselves have no fields to walk (getFieldCount() == 0), so this is
        // a no-op for those and safe to call unconditionally.
        reflect_fields_( type_layout, 0, info.name, info.binding_space, p_module.stage, info.fields, p_module.params );

        p_module.params.push_back( std::move( info ) );
    }
}

void SlangModuleCompiler::reflect_entry_point_params_(
    slang::EntryPointLayout* ep_layout, gfx::ShaderStage stage, DynamicArray<ShaderParamDesc>& out_params,
    gfx::VertexLayout& out_vert_layout
)
{
    if (!ep_layout)
        return;

    DynamicArray<ShaderParamDesc> varying_inputs;

    for (unsigned j = 0; j < ep_layout->getParameterCount(); j++) {
        auto param = ep_layout->getParameterByIndex( j );
        if (!param)
            continue;

        auto type_layout = param->getTypeLayout();
        if (!type_layout)
            continue;

        // Classify the parameter by its binding category.
        bool has_varying_input       = false;
        bool has_constant_buffer     = false;
        bool has_push_constant       = false;
        bool has_resource_or_sampler = false;

        for (unsigned c = 0; c < param->getCategoryCount(); c++) {
            switch (param->getCategoryByIndex( c )) {
                case slang::ParameterCategory::VaryingInput:
                    has_varying_input = true;
                    break;
                case slang::ParameterCategory::ConstantBuffer:
                    has_constant_buffer = true;
                    break;
                case slang::ParameterCategory::PushConstantBuffer:
                    has_push_constant = true;
                    break;
                case slang::ParameterCategory::SamplerState:
                case slang::ParameterCategory::ShaderResource:
                case slang::ParameterCategory::UnorderedAccess:
                    has_resource_or_sampler = true;
                    break;
                default:
                    break;
            }
        }

        // --- Varying inputs (vertex attributes) ---
        if (has_varying_input) {
            if (type_layout->getKind() == slang::TypeReflection::Kind::Struct) {
                uint32_t running_offset = 0;

                for (unsigned f = 0; f < type_layout->getFieldCount(); f++) {
                    auto field = type_layout->getFieldByIndex( f );
                    if (!field)
                        continue;
                    auto field_type = field->getTypeLayout();
                    if (!field_type)
                        continue;

                    auto* sem = field->getSemanticName();
                    if (sem && _strnicmp( sem, "SV_", 3 ) == 0)
                        continue;

                    ShaderParamDesc info;
                    info.name          = field->getName() ? field->getName() : "";
                    info.semantic_name = sem ? sem : "";
                    info.resource_type = gfx::ResourceType::VARYING_INPUT;
                    info.param_type    = translate_slang_type( field_type->getType() );
                    info.location      = static_cast<uint32_t>( field->getBindingIndex() );
                    info.offset        = running_offset;
                    info.binding_idx   = 0;
                    varying_inputs.push_back( std::move( info ) );

                    running_offset += static_cast<uint32_t>( field_type->getSize() );
                }

                uint32_t vertex_stride = static_cast<uint32_t>( type_layout->getSize() );
                for (auto& p : varying_inputs)
                    p.element_stride = vertex_stride;

            } else {
                auto* sem = param->getSemanticName();
                if (sem && _strnicmp( sem, "SV_", 3 ) == 0)
                    continue;

                ShaderParamDesc info;
                info.name           = param->getName() ? param->getName() : "";
                info.semantic_name  = sem ? sem : "";
                info.resource_type  = gfx::ResourceType::VARYING_INPUT;
                info.param_type     = translate_slang_type( type_layout->getType() );
                info.location       = static_cast<uint32_t>( param->getBindingIndex() );
                info.offset         = 0;
                info.binding_idx    = 0;
                info.element_stride = static_cast<uint32_t>( type_layout->getSize() );
                varying_inputs.push_back( std::move( info ) );
            }
            continue;
        }

        // --- Push constants / uniform constant buffers ---
        if (has_push_constant || has_constant_buffer) {
            auto kind = type_layout->getKind();

            // Unwrap ConstantBuffer / ParameterBlock to get the element type
            if (kind == slang::TypeReflection::Kind::ConstantBuffer ||
                kind == slang::TypeReflection::Kind::ParameterBlock) {
                type_layout = type_layout->getElementTypeLayout();
            }

            ShaderParamDesc info;
            info.name       = param->getName() ? param->getName() : "";
            info.stage_mask = stage;

            if (has_push_constant) {
                info.resource_type = gfx::ResourceType::CONSTANT_BUFFER;
                info.flags         = gfx::ResourceFlags::INLINE_CONSTANTS;
                info.binding_space = static_cast<gfx::BindingSet>( param->getBindingSpace() );
                info.binding_idx   = param->getBindingIndex();
            } else {
                info.resource_type = gfx::ResourceType::CONSTANT_BUFFER;
                info.element_stride =
                    type_layout->getElementStride( SlangParameterCategory::SLANG_PARAMETER_CATEGORY_CONSTANT_BUFFER );
                info.total_size_bytes = type_layout->getSize();
                info.binding_space    = static_cast<gfx::BindingSet>( param->getBindingSpace() );
                info.binding_idx      = param->getBindingIndex();
            }

            NC_LOG_DEBUG_C(
                log::IO, "EP param '{}' category='{}' push={} flags={}", info.name,
                has_push_constant ? "PushConstant" : "ConstantBuffer", has_push_constant,
                static_cast<uint32_t>( info.flags )
            );

            reflect_fields_( type_layout, 0, info.name, info.binding_space, stage, info.fields, out_params );

            out_params.push_back( std::move( info ) );
            continue;
        }

        // --- Resources / samplers nested in the EP scope ---
        if (has_resource_or_sampler) {
            ShaderParamDesc info;
            info.name          = param->getName() ? param->getName() : "";
            info.binding_space = static_cast<gfx::BindingSet>( param->getBindingSpace() );
            info.binding_idx   = param->getBindingIndex();
            info.stage_mask    = stage;

            auto kind = type_layout->getKind();
            if (kind == slang::TypeReflection::Kind::Resource) {
                info.resource_type = classify_resource_type( type_layout->getType() );
                info.param_type    = translate_slang_type( type_layout->getType() );
                auto slang_access  = type_layout->getType()->getResourceAccess();
                info.cpu_access    = ( slang_access == SLANG_RESOURCE_ACCESS_READ_WRITE )
                                         ? gfx::ResourceCpuAccessFlags::READ_WRITE
                                         : gfx::ResourceCpuAccessFlags::READ;
            } else if (kind == slang::TypeReflection::Kind::SamplerState) {
                info.resource_type = gfx::ResourceType::SAMPLER;
            } else {
                info.resource_type = gfx::ResourceType::CONSTANT_BUFFER;
            }

            NC_LOG_DEBUG_C(
                log::IO, "EP resource '{}' type='{}' space={} idx={}", info.name,
                rtti::get_enum_name( &info.resource_type ), info.binding_space, info.binding_idx
            );

            out_params.push_back( std::move( info ) );
            continue;
        }

        NC_LOG_DEBUG_C(
            log::IO, "EP param '{}' skipped (unrecognized category count={})", param->getName(),
            param->getCategoryCount()
        );
    }

    // Build vertex p_module_layout from varying inputs
    out_vert_layout.reserve( out_vert_layout.size() + varying_inputs.size() );
    for (const auto& p : varying_inputs) {
        gfx::VertexLayoutElement elem;
        elem.location        = p.location;
        elem.type            = p.param_type;
        elem.buffer_slot     = p.binding_idx;
        elem.stride          = static_cast<uint32_t>( p.element_stride );
        elem.relative_offset = static_cast<uint32_t>( p.offset );
        elem.normalized      = false;
        elem.frequency       = gfx::VertexFrequency::PER_VERTEX;
        out_vert_layout.push_back( elem );
    }

    NC_LOG_DEBUG_C( log::IO, "vertex layout (reflected, {} elements):", out_vert_layout.size() );
    for (auto& ve : out_vert_layout) {
        NC_LOG_DEBUG_C(
            log::IO, "   slot={} loc={} type={} rel_offset={} stride={} semantic='{}'", ve.buffer_slot, ve.location,
            rtti::get_enum_name( &ve.type ), ve.relative_offset, ve.stride,
            ve.hlsl_semantic.data() ? ve.hlsl_semantic : "N/A"
        );
    }

    for (auto& p : varying_inputs)
        out_params.push_back( std::move( p ) );
}

// ---------------------------------------------------------------------------
// Material attribute reflection
// ---------------------------------------------------------------------------

static int get_int_attr_( slang::TypeReflection* type, const char* name, int fallback = 0 )
{
    auto* attr = type->findUserAttributeByName( name );
    if (!attr)
        return fallback;
    int val = fallback;
    if (attr->getArgumentCount() > 0)
        attr->getArgumentValueInt( 0, &val );
    return val;
}

void SlangModuleCompiler::reflect_material_attributes_( slang::IModule* p_module, MaterialShaderDesc& out_desc )
{
    if (!p_module || !compile_session)
        return;

    auto* module_reflection = p_module->getModuleReflection();
    if (!module_reflection)
        return;

    for (unsigned i = 0; i < module_reflection->getChildrenCount(); i++) {
        auto* child = module_reflection->getChild( i );
        if (!child || child->getKind() != slang::DeclReflection::Kind::Struct)
            continue;

        auto* type = child->getType();
        if (!type)
            continue;

        // Check for any material attribute — if found, this is a material struct
        bool has_any = type->findUserAttributeByName( "CullMode" ) || type->findUserAttributeByName( "FillMode" ) ||
                       type->findUserAttributeByName( "DepthTest" ) || type->findUserAttributeByName( "DepthWrite" ) ||
                       type->findUserAttributeByName( "Blend" );
        if (!has_any)
            continue;

        out_desc.is_material   = true;
        out_desc.material_type = type;
        out_desc.cull_mode     = static_cast<gfx::CullMode>( get_int_attr_( type, "CullMode", 0 ) );
        out_desc.fill_mode     = static_cast<gfx::FillMode>( get_int_attr_( type, "FillMode", 0 ) );
        out_desc.depth_test    = get_int_attr_( type, "DepthTest", 0 ) != 0;
        out_desc.depth_write   = get_int_attr_( type, "DepthWrite", 0 ) != 0;
        out_desc.blend         = static_cast<gfx::BlendPreset>( get_int_attr_( type, "Blend", 1 ) );

        // Infer Spatial vs Canvas from the material interface: ISpatialMaterial's
        // fragment() returns SurfaceOutput (a struct), ICanvasMaterial's returns float4
        // (a vector).
        out_desc.material_kind = MaterialShaderType::Spatial;
        for (unsigned c = 0; c < child->getChildrenCount(); ++c) {
            auto* member = child->getChild( c );
            if (!member || member->getKind() != slang::DeclReflection::Kind::Func)
                continue;
            auto* fn = member->asFunction();
            if (!fn || !fn->getName() || std::strcmp( fn->getName(), "fragment" ) != 0)
                continue;
            auto* ret = fn->getReturnType();
            if (ret && ret->getKind() == slang::TypeReflection::Kind::Vector)
                out_desc.material_kind = MaterialShaderType::Canvas;
            break;
        }

        NC_LOG_DEBUG_C(
            log::IO,
            "reflect_material_attributes: struct '{}' cull={} fill={} depth_test={} depth_write={} blend={} kind={}",
            child->getName(), static_cast<int>( out_desc.cull_mode ), static_cast<int>( out_desc.fill_mode ),
            out_desc.depth_test, out_desc.depth_write, static_cast<int>( out_desc.blend ),
            static_cast<int>( out_desc.material_kind )
        );
        break; // take the first material struct found
    }
}

} // namespace nc
