#include "shader_compiler.h"

#include <filesystem>

#include <ncore/services/video/rhi_types.h>
#include <ncore/utils/assert.h>
#include <ncore/utils/log.h>

#include <slang-com-helper.h>

namespace nc {

namespace {

static rhi::ShaderValueType translate_slang_type( slang::TypeReflection* type )
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
            return rhi::ShaderValueType::FLOAT;
        if (is_int)
            return rhi::ShaderValueType::INT;
        if (is_bool)
            return rhi::ShaderValueType::BOOL;
        return rhi::ShaderValueType::UNKNOWN;
    }

    if (kind == slang::TypeReflection::Kind::Vector) {
        if (is_float) {
            switch (element_count) {
                case 2:
                    return rhi::ShaderValueType::FLOAT2;
                case 3:
                    return rhi::ShaderValueType::FLOAT3;
                case 4:
                    return rhi::ShaderValueType::FLOAT4;
            }
        }
        if (is_int) {
            switch (element_count) {
                case 2:
                    return rhi::ShaderValueType::INT2;
                case 3:
                    return rhi::ShaderValueType::INT3;
                case 4:
                    return rhi::ShaderValueType::INT4;
            }
        }
        if (is_uint16 && element_count == 4)
            return rhi::ShaderValueType::USHORT4;
        return rhi::ShaderValueType::UNKNOWN;
    }

    if (kind == slang::TypeReflection::Kind::Matrix) {
        if (is_float && row_count == 4 && col_count == 4)
            return rhi::ShaderValueType::MAT4;
        NC_LOG_WARN_C(
            log::GRAPHICS, "translate_slang_type: unsupported matrix shape {}x{}, scalar={}", row_count, col_count,
            static_cast<int>( scalar )
        );
        return rhi::ShaderValueType::UNKNOWN;
    }

    if (kind == slang::TypeReflection::Kind::Resource) {
        auto shape      = type->getResourceShape();
        auto base_shape = shape & SLANG_RESOURCE_BASE_SHAPE_MASK;
        const bool is_array = ( shape & SLANG_TEXTURE_ARRAY_FLAG ) != 0;

        switch (base_shape) {
            case SLANG_TEXTURE_2D:
                return is_array ? rhi::ShaderValueType::TEXTURE_2D_ARRAY : rhi::ShaderValueType::TEXTURE_2D;
            case SLANG_TEXTURE_CUBE:
                return rhi::ShaderValueType::TEXTURE_CUBED;
            case SLANG_TEXTURE_1D:
            case SLANG_TEXTURE_3D:
                return rhi::ShaderValueType::TEXTURE_2D;
            default:
                break;
        }
    }

    if (kind == slang::TypeReflection::Kind::SamplerState) {
        return rhi::ShaderValueType::SAMPLER;
    }

    return rhi::ShaderValueType::UNKNOWN;
}

static rhi::ResourceType classify_resource_type( slang::TypeReflection* type )
{
    auto shape      = type->getResourceShape();
    auto base_shape = shape & SLANG_RESOURCE_BASE_SHAPE_MASK;

    switch (base_shape) {
        case SLANG_STRUCTURED_BUFFER:
            if (type->getResourceAccess() == SLANG_RESOURCE_ACCESS_READ_WRITE) {
                return rhi::ResourceType::BUFFER_UAV;
            }
            [[fallthrough]];
        case SLANG_BYTE_ADDRESS_BUFFER:
            return rhi::ResourceType::BUFFER_SRV;

        case SLANG_TEXTURE_1D:
        case SLANG_TEXTURE_2D:
        case SLANG_TEXTURE_3D:
        case SLANG_TEXTURE_CUBE:
        case SLANG_TEXTURE_BUFFER:
            if (type->getResourceAccess() == SLANG_RESOURCE_ACCESS_READ_WRITE)
                return rhi::ResourceType::TEXTURE_UAV;
            return rhi::ResourceType::TEXTURE_SRV;

        default:
            NC_LOG_WARN_C(
                log::GRAPHICS, "classify_resource_type: unhandled resource base shape {}",
                static_cast<int>( base_shape )
            );
            return rhi::ResourceType::TEXTURE_SRV;
    }
}
