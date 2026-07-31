#include "shader_compiler.h"

#include <ncore/modules/video/rhi_types.h>
#include <ncore/utils/log.h>

namespace nc {

ShaderCompiler::ShaderCompiler()
{
    SlangResult r = slang::createGlobalSession( global_session.writeRef() );
    if (SLANG_FAILED( r )) {
        NC_LOG_ERROR_C( log::GRAPHICS, "ShaderCompiler: failed to create Slang global session" );
    }

    NC_LOG_TRACE_C( log::GRAPHICS, "ShaderCompiler initialized" );
}

ShaderCompiler::~ShaderCompiler()
{
    slang::shutdown();
}

static ShaderValueType translate_slang_type( slang::TypeReflection* type )
{
    auto kind   = type->getKind();
    auto scalar = type->getScalarType();

    size_t element_count = 1, row_count = 1, col_count = 1;
    if (kind == slang::TypeReflection::Kind::Vector) {
        element_count = type->getElementCount();
    } else if (kind == slang::TypeReflection::Kind::Matrix) {
        row_count = type->getRowCount();
        col_count = type->getColumnCount();
    } else if (kind == slang::TypeReflection::Kind::Array) {
        element_count = type->getElementCount();
        NC_LOG_WARN_C( log::IO, "Array field '{}' not fully supported, treating element type only", type->getName() );
    }

    auto is_float  = scalar == slang::TypeReflection::ScalarType::Float32;
    auto is_int    = scalar == slang::TypeReflection::ScalarType::Int32;
    auto is_bool   = scalar == slang::TypeReflection::ScalarType::Bool;
    auto is_uint16 = scalar == slang::TypeReflection::ScalarType::UInt16;

    if (kind == slang::TypeReflection::Kind::Scalar) {
        if (is_float)
            return ShaderValueType::FLOAT;
        if (is_int)
            return ShaderValueType::INT;
        if (is_bool)
            return ShaderValueType::BOOL;
        return ShaderValueType::UNKNOWN;
    }

    if (kind == slang::TypeReflection::Kind::Vector) {
        if (is_float) {
            switch (element_count) {
                case 2:
                    return ShaderValueType::FLOAT2;
                case 3:
                    return ShaderValueType::FLOAT3;
                case 4:
                    return ShaderValueType::FLOAT4;
            }
        }
        if (is_int) {
            switch (element_count) {
                case 2:
                    return ShaderValueType::INT2;
                case 3:
                    return ShaderValueType::INT3;
                case 4:
                    return ShaderValueType::INT4;
            }
        }
        if (is_uint16 && element_count == 4)
            return ShaderValueType::USHORT4;
        return ShaderValueType::UNKNOWN;
    }

    if (kind == slang::TypeReflection::Kind::Matrix) {
        if (is_float && row_count == 4 && col_count == 4)
            return ShaderValueType::MAT4;
        NC_LOG_WARN_C(
            log::GRAPHICS, "translate_slang_type: unsupported matrix shape {}x{}, scalar={}", row_count, col_count,
            static_cast<int>( scalar )
        );
        return ShaderValueType::UNKNOWN;
    }

    if (kind == slang::TypeReflection::Kind::Resource) {
        auto shape = type->getResourceShape();
        if (shape == SLANG_TEXTURE_2D)
            return ShaderValueType::TEXTURE2D;
    }

    if (kind == slang::TypeReflection::Kind::SamplerState) {
        return ShaderValueType::SAMPLER;
    }

    return ShaderValueType::UNKNOWN;
}

static ResourceType classify_resource_type( slang::TypeReflection* type )
{
    auto shape      = type->getResourceShape();
    auto base_shape = shape & SLANG_RESOURCE_BASE_SHAPE_MASK;

    switch (base_shape) {
        case SLANG_STRUCTURED_BUFFER:
        case SLANG_BYTE_ADDRESS_BUFFER:
            return ResourceType::BUFFER_SRV;

        case SLANG_TEXTURE_1D:
        case SLANG_TEXTURE_2D:
        case SLANG_TEXTURE_3D:
        case SLANG_TEXTURE_CUBE:
        case SLANG_TEXTURE_BUFFER:
            return ResourceType::TEXTURE_SRV;

        default:
            NC_LOG_WARN_C(
                log::GRAPHICS, "classify_resource_type: unhandled resource base shape {}",
                static_cast<int>( base_shape )
            );
            return ResourceType::TEXTURE_SRV;
    }
}

ShaderCompileResult ShaderCompiler::compile( const ShaderCompileDesc& desc )
{
    ShaderCompileResult result;
    result.ok = false;

    if (desc.filepath.empty()) {
        NC_LOG_ERROR_C( log::IO, "Filepath is required for compilation" );
        return result;
    }

    if (!ensure_session( desc.recreate_session ))
        return result;

    slang::IModule* module = nullptr;
    {
        Slang::ComPtr<slang::IBlob> diags;
        module = compile_session->loadModule( desc.filepath.data(), diags.writeRef() );

        if (diags && diags->getBufferSize() > 0) {
            auto diags_str     = static_cast<const char*>( diags->getBufferPointer() );
            result.diagnostics = diags_str;
        }

        if (!module) {
            return result;
        }
    }

    // See also https://docs.shader-slang.org/en/latest/compilation-api.html#get-target-kernel-code

    for (SlangInt32 i = 0; i < module->getDefinedEntryPointCount(); i++) {
        Slang::ComPtr<slang::IEntryPoint> ep;
        if (SLANG_FAILED( module->getDefinedEntryPoint( i, ep.writeRef() ) )) {
            NC_LOG_ERROR_C( log::IO, "Getting entry point at idx '{}' failed", i );
            continue;
        }

        slang::IComponentType* components[] = { module, ep.get() };
        Slang::ComPtr<slang::IComponentType> composed;
        {
            Slang::ComPtr<slang::IBlob> diags;
            auto r =
                compile_session->createCompositeComponentType( components, 2, composed.writeRef(), diags.writeRef() );
            if (diags && diags->getBufferSize() > 0)
                result.diagnostics += static_cast<const char*>( diags->getBufferPointer() );
            if (SLANG_FAILED( r ))
                continue;
        }

        Slang::ComPtr<slang::IComponentType> linked;
        {
            Slang::ComPtr<slang::IBlob> diags;
            auto r = composed->link( linked.writeRef(), diags.writeRef() );
            if (diags && diags->getBufferSize() > 0)
                result.diagnostics += static_cast<const char*>( diags->getBufferPointer() );
            if (SLANG_FAILED( r ))
                continue;
        }

        Slang::ComPtr<slang::IBlob> spirv_code;
        {
            Slang::ComPtr<slang::IBlob> diags;
            auto r = linked->getEntryPointCode( 0, 0, spirv_code.writeRef(), diags.writeRef() );
            if (diags && diags->getBufferSize() > 0)
                result.diagnostics += static_cast<const char*>( diags->getBufferPointer() );
            if (SLANG_FAILED( r ))
                continue;

            NC_LOG_INFO_C(
                log::IO, "Shader '{}' entry point '{}' compiled (stage={}, {} bytes)", desc.filepath,
                ep->getFunctionReflection()->getName(),
                ep->getLayout()->getEntryPointByIndex( 0 )->getStage() == SLANG_STAGE_VERTEX     ? "VS"
                : ep->getLayout()->getEntryPointByIndex( 0 )->getStage() == SLANG_STAGE_FRAGMENT ? "PS"
                                                                                                 : "?",
                spirv_code->getBufferSize()
            );
        }

        ShaderDesc entry_out;
        entry_out.entrypoint = ep->getFunctionReflection()->getName();

#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wswitch-enum"
        switch (ep->getLayout()->getEntryPointByIndex( 0 )->getStage()) {
            case SLANG_STAGE_VERTEX:
                entry_out.stage = ShaderType::VERTEX;
                break;
            case SLANG_STAGE_FRAGMENT:
                entry_out.stage = ShaderType::PIXEL;
                break;
            default:
                entry_out.stage = ShaderType::MULTIPLE;
                break;
        }
#pragma clang diagnostic pop

        auto words   = static_cast<const uint32_t*>( spirv_code->getBufferPointer() );
        size_t count = spirv_code->getBufferSize() / sizeof( uint32_t );
        entry_out.bytecode.assign( words, words + count );

        auto layout = linked->getLayout();
        if (!layout) {
            NC_LOG_ERROR_C( log::IO, "Failed to get layout for '{}'", entry_out.entrypoint );
            return result;
        }

        for (unsigned j = 0; j < layout->getParameterCount(); j++) {
            auto param = layout->getParameterByIndex( j );
            if (!param)
                continue;

            auto type_layout = param->getTypeLayout();
            if (!type_layout)
                continue;

            auto original_kind = type_layout->getKind();

            if (type_layout->getKind() == slang::TypeReflection::Kind::ConstantBuffer ||
                type_layout->getKind() == slang::TypeReflection::Kind::ParameterBlock) {
                type_layout = type_layout->getElementTypeLayout();
            }

            NC_LOG_DEBUG_C(
                log::IO, "param '{}' category={} field_count={}", param->getName(),
                static_cast<int>( param->getCategory() ), type_layout->getFieldCount()
            );

            ShaderParamInfo info;
            info.name          = param->getName() ? param->getName() : "";
            info.semantic_name = param->getSemanticName() ? param->getSemanticName() : "";
            info.binding_idx   = param->getBindingIndex();
            info.binding_space = param->getBindingSpace();

#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wswitch-enum"
            switch (original_kind) {
                case slang::TypeReflection::Kind::ConstantBuffer:
                    info.resource_type = ResourceType::CONSTANT_BUFFER;
                    break;
                case slang::TypeReflection::Kind::Resource:
                    info.resource_type = classify_resource_type( type_layout->getType() );
                    break;
                case slang::TypeReflection::Kind::SamplerState:
                    info.resource_type = ResourceType::SAMPLER;
                    break;
                default:
                    info.resource_type = ResourceType::CONSTANT_BUFFER;
                    break;
            }
#pragma clang diagnostic pop

            for (unsigned f = 0; f < type_layout->getFieldCount(); f++) {
                auto field = type_layout->getFieldByIndex( f );
                if (!field)
                    continue;
                auto field_type = field->getTypeLayout();

                ShaderParamField out;
                out.name   = field->getName() ? field->getName() : "";
                out.offset = static_cast<uint32_t>( field->getOffset() );
                out.size   = static_cast<uint32_t>( field_type->getSize() );
                out.stride = field_type->getStride();
                out.type   = translate_slang_type( field_type->getType() );
                info.fields.push_back( out );

                NC_LOG_DEBUG_C(
                    log::IO, "  field '{}': type={} offset={} size={} stride={}", out.name,
                    static_cast<int>( out.type ), out.offset, out.size, out.stride
                );
            }

            entry_out.params.push_back( std::move( info ) );
        }

        if (entry_out.stage == ShaderType::VERTEX) {
            auto ep_layout = layout->getEntryPointByIndex( 0 );
            if (!ep_layout)
                continue;

            for (unsigned j = 0; j < ep_layout->getParameterCount(); j++) {
                auto param = ep_layout->getParameterByIndex( j );
                if (!param)
                    continue;

                bool has_varying_input = false;
                for (unsigned c = 0; c < param->getCategoryCount(); c++) {
                    if (param->getCategoryByIndex( c ) == slang::ParameterCategory::VaryingInput) {
                        has_varying_input = true;
                        break;
                    }
                }
                if (!has_varying_input)
                    continue;

                auto type_layout = param->getTypeLayout();
                if (!type_layout)
                    continue;

                if (type_layout->getKind() == slang::TypeReflection::Kind::Struct) {
                    for (unsigned f = 0; f < type_layout->getFieldCount(); f++) {
                        auto field = type_layout->getFieldByIndex( f );
                        if (!field)
                            continue;
                        auto field_type = field->getTypeLayout();
                        if (!field_type)
                            continue;

                        ShaderParamInfo info;
                        info.name = field->getName() ? field->getName() : "";
                        auto* sem = field->getSemanticName();
                        if (sem && _strnicmp( sem, "SV_", 3 ) == 0)
                            continue;

                        info.semantic_name = sem ? sem : "";
                        info.resource_type = ResourceType::VARYING_INPUT;
                        info.value_type    = translate_slang_type( field->getTypeLayout()->getType() );
                        info.location      = static_cast<uint32_t>( field->getSemanticIndex() );
                        info.offset        = field->getOffset( slang::ParameterCategory::VaryingInput );
                        info.binding_idx   = field->getBindingIndex();
                        info.stride        = field_type->getStride();
                        entry_out.params.push_back( std::move( info ) );
                    }
                } else {
                    auto* sem = param->getSemanticName();
                    if (!( sem && _strnicmp( sem, "SV_", 3 ) == 0 )) {
                        ShaderParamInfo info;
                        info.name          = param->getName() ? param->getName() : "";
                        info.semantic_name = sem ? sem : "";
                        info.resource_type = ResourceType::VARYING_INPUT;
                        info.value_type    = translate_slang_type( type_layout->getType() );
                        info.location      = static_cast<uint32_t>( param->getSemanticIndex() );
                        info.offset        = 0;
                        info.binding_idx   = param->getBindingIndex();
                        info.stride        = type_layout->getStride();
                        entry_out.params.push_back( std::move( info ) );
                    }
                }

                for (const auto& p : entry_out.params) {
                    if (p.resource_type != ResourceType::VARYING_INPUT)
                        continue;

                    VertexLayoutElement elem;
                    elem.location        = p.location;
                    elem.type            = p.value_type;
                    elem.buffer_slot     = p.binding_idx;
                    elem.stride          = p.stride;
                    elem.relative_offset = static_cast<uint32_t>( p.offset );
                    elem.normalized      = false;
                    elem.frequency       = VertexFrequency::PER_VERTEX;
                    entry_out.vert_layout.push_back( elem );
                }
            }

            NC_LOG_DEBUG_C( log::IO, "  vertex layout (reflected, {} elements):", entry_out.vert_layout.size() );
            for (auto& ve : entry_out.vert_layout) {
                NC_LOG_DEBUG_C(
                    log::IO, "    slot={} loc={} type={} offset={} stride={} semantic='{}'", ve.buffer_slot,
                    ve.location, static_cast<int>( ve.type ), ve.relative_offset, ve.stride,
                    ve.hlsl_semantic ? ve.hlsl_semantic : ""
                );
            }
        }

        result.programs.push_back( std::move( entry_out ) );
    }

    result.shader_name = desc.filepath;
    result.ok          = true;
    return result;
}

bool ShaderCompiler::ensure_session( bool recreate_session )
{
    if (session_created && !recreate_session)
        return true;

    if (!global_session) {
        NC_LOG_ERROR_C( log::GRAPHICS, "ShaderCompiler: no global session available" );
        return false;
    }

    slang::SessionDesc sd;
    slang::TargetDesc target;
    target.format              = SLANG_SPIRV;
    target.profile             = global_session->findProfile( "spirv_1_5" );
    sd.targets                 = &target;
    sd.targetCount             = 1;
    sd.defaultMatrixLayoutMode = SlangMatrixLayoutMode::SLANG_MATRIX_LAYOUT_COLUMN_MAJOR;
    sd.searchPaths             = search_paths;
    sd.searchPathCount         = 1;

    if (SLANG_FAILED( global_session->createSession( sd, compile_session.writeRef() ) )) {
        NC_LOG_ERROR_C( log::GRAPHICS, "ShaderCompiler: failed to create compile session" );
        return false;
    }

    session_created = true;
    return true;
}

} // namespace nc
