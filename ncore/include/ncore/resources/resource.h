// Copyright (C) 2026 Ahmad Ghalib Athariq <alib.athariq@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

#pragma once

#include <fstream>
#include <string>

#include <ncore/core/object.h>
#include <ncore/core/reference.h>
#include <ncore/core/rid.h>
#include <ncore/services/video/gfx_types.h> // FIXME: if there's way we don't depend on service layer that'd be great

namespace nc {

// ------------------------------------------------------------------------------
// ResourceFormatID
// ------------------------------------------------------------------------------

/**
 * @brief ResourceFormatID uniquely identifies a resource format (e.g. image, etc).
 */
struct REFLECT NCAPI ResourceFormatID {
    REFLECT uint32_t id = 0;

    ResourceFormatID() = default;
    ResourceFormatID( StringView ascii_id );
    ResourceFormatID( const char* ascii_id );

    bool is_valid() const;
    String to_string() const;
};

// ------------------------------------------------------------------------------
// IResource
// ------------------------------------------------------------------------------

/**
 * @brief IResource is any shareable and reference-counted resource that lives on RAM.
 *
 * Supports binary serialization for offline compilation and runtime loading.
 *
 * Binary format: [4-byte magic] [4-byte version] [4-byte payload size] [payload...]
 */
class NCAPI IResource : public RefCounted {
    NCLASS( IResource, RefCounted )

public:
    virtual ResourceFormatID get_format_id() const
    {
        return ResourceFormatID();
    }

    virtual uint32_t get_version() const
    {
        return 1;
    }

    virtual size_t get_size_bytes() const
    {
        return 0;
    }

    /**
     * @brief Rebuild derived (non-REFLECT) state after binary deserialization.
     *
     * Called by ResourceArchive::deserialize() once all REFLECT fields have
     * been read. Default no-op; Shader overrides it to rebuild lookups that
     * its constructors normally populate.
     */
    virtual void on_deserialized() {}

    String filepath = "generated";
    RID rid         = 0;
};

// ------------------------------------------------------------------------------
// Shader, MaterialShader
// ------------------------------------------------------------------------------

struct REFLECT NCAPI ShaderParamField {
    REFLECT String name;
    REFLECT uint32_t offset;
    REFLECT uint32_t size;
    REFLECT size_t stride;
    REFLECT gfx::ParameterType type;
    REFLECT uint32_t array_size;
};

struct REFLECT NCAPI ShaderParamDesc {
    REFLECT String name;
    REFLECT String semantic_name;
    REFLECT gfx::ResourceType resource_type        = gfx::ResourceType::CONSTANT_BUFFER;
    REFLECT gfx::ParameterType param_type          = gfx::ParameterType::UNKNOWN;
    REFLECT gfx::ResourceFlags flags               = gfx::ResourceFlags::NONE;
    REFLECT gfx::ResourceCpuAccessFlags cpu_access = gfx::ResourceCpuAccessFlags::READ;
    REFLECT gfx::BindingSet binding_space          = 0; // or "set" in Vulkan terms.
    REFLECT uint32_t binding_idx                   = 0; // the "binding/register" on the binding_space.
    REFLECT uint32_t location                      = 0; // relevant only for the varying_input resource type.
    REFLECT size_t offset                          = 0;
    REFLECT size_t element_stride                  = 0;
    REFLECT size_t total_size_bytes                = 0;
    REFLECT DynamicArray<ShaderParamField> fields;
    REFLECT gfx::ShaderStage stage_mask = gfx::ShaderStage::NONE; // bitmask of all stages using this param.
};

using ShaderParamLayout = DynamicArray<ShaderParamDesc>;

/**
 * @brief Attributes of a shader program.
 */
struct REFLECT NCAPI ShaderDesc {
    REFLECT String name;
    REFLECT gfx::ShaderStage stage;
    REFLECT String entrypoint;
    REFLECT DynamicArray<uint32_t> bytecode;
    REFLECT ShaderParamLayout params;
    REFLECT gfx::VertexLayout vert_layout;
    REFLECT uint32_t num_threads_x = 1;
    REFLECT uint32_t num_threads_y = 1;
    REFLECT uint32_t num_threads_z = 1;
};

/**
 * @brief Shader resource represents compiled GPU program(s)
 * or a multi-stage pipeline.
 */
class NCAPI Shader : public IResource {
    NCLASS( Shader, IResource )

public:
    Shader() = default;
    /**
     * @brief Construct single-stage shader.
     */
    Shader( const ShaderDesc& p_desc );
    /**
     * @brief Construct multi-stage shader.
     */
    Shader( DynamicArray<ShaderDesc> p_stages_desc );

    ResourceFormatID get_format_id() const override;
    size_t get_size_bytes() const override;
    void on_deserialized() override;

    /**
     * @brief Get the shader's entrypoint name.
     */
    StringView get_entry_point( gfx::ShaderStage stage ) const [[clang::lifetimebound]];

    /**
     * @brief Get the shader's compiled bytecode.
     */
    Span<const uint32_t> get_bytecode( gfx::ShaderStage stage ) const [[clang::lifetimebound]];

    /**
     * @brief Return shader parameter descriptions from all available stages.
     * @return Array view of the reflected parameter descriptions.
     */
    Span<const ShaderParamDesc> get_params() const [[clang::lifetimebound]];
    /**
     * @brief Get specific set of shader parameter descriptions from all available stages.
     * @param p_binding_set The shader's binding set.
     * @return Array view of the matching reflected parameter descriptions.
     */
    Span<const ShaderParamDesc> get_param_set( gfx::BindingSet p_binding_set ) const;
    /**
     * @brief Find a shader parameter info by name.
     */
    const ShaderParamDesc* find_param( StringView p_name ) const [[clang::lifetimebound]];

    bool has_stage( gfx::ShaderStage stage ) const;
    const ShaderDesc* get_stage_desc( gfx::ShaderStage stage ) const;

    /**
     * @brief Get each stage attributes making up this shader.
     */
    Span<const ShaderDesc> get_stages() const [[clang::lifetimebound]];

    /**
     * @brief Return the shader stages of this shader as a mask.
     */
    gfx::ShaderStage get_stage_flags() const;

protected:
    void build_configs_();

    REFLECT DynamicArray<ShaderDesc> stages;
    ShaderParamLayout unified_params;
    HashMap<gfx::BindingSet, Span<const ShaderParamDesc>> param_set_lookup;
    HashMap<StringView, size_t> param_name_lookup;
};

enum class MaterialShaderType {
    Spatial,
    Canvas
};

/**
 * @brief MaterialShader is a shader that has been given a specific contract
 * with NCORE's material system.
 *
 * Loosely inspired by EsotericaEngine's MaterialShader concept.
 */
class NCAPI MaterialShader : public Shader {
    NCLASS( MaterialShader, Shader )

public:
    MaterialShader() = default;
    MaterialShader( DynamicArray<ShaderDesc> p_stages_desc ) : Shader( std::move( p_stages_desc ) ) {}

    ResourceFormatID get_format_id() const override;

    REFLECT MaterialShaderType material_type            = MaterialShaderType::Spatial;
    REFLECT gfx::CullMode cull_mode                     = gfx::CullMode::NONE;
    REFLECT gfx::FillMode fill_mode                     = gfx::FillMode::SOLID;
    REFLECT bool depth_test                             = false;
    REFLECT bool depth_write                            = false;
    REFLECT gfx::BlendPreset blend                      = gfx::BlendPreset::ALPHA_BLEND;
    REFLECT gfx::MultisampleStateDesc multisample_state = { 1, 0 };
};

// ------------------------------------------------------------------------------
// Mesh, CubeMesh, PlaneMesh
// ------------------------------------------------------------------------------

struct REFLECT NCAPI MeshDesc {
    REFLECT BytesBuffer vertices;
    REFLECT DynamicArray<uint16_t> indices;
    REFLECT uint32_t vertex_stride;
};

/**
 * @brief Mesh is a static geometrical data containing vertex attributes.
 */
class NCAPI Mesh : public IResource {
    NCLASS( Mesh, IResource )

public:
    Mesh( const MeshDesc& p_desc ) : desc( p_desc ) {}

    ResourceFormatID get_format_id() const override;
    size_t get_size_bytes() const override;

    Span<const std::byte> get_vertices() const [[clang::lifetimebound]];
    Span<const uint16_t> get_indices() const [[clang::lifetimebound]];
    size_t vertex_count() const;
    size_t index_count() const;
    uint32_t get_vertex_stride() const;

protected:
    REFLECT MeshDesc desc;
};

/**
 * @brief A primitve cube mesh.
 */
class NCAPI CubeMesh : public Mesh {
public:
    CubeMesh();
};

/**
 * @brief A primitve plane mesh.
 *
 * The plane lies on the XZ plane (normal +Y) with a unit size of 2x2 (-1..1),
 * subdivided into x_segments * z_segments quads. Limited to 255 segments per
 * side due to 16-bit indices.
 */
class NCAPI PlaneMesh : public Mesh {
public:
    PlaneMesh( uint32_t x_segments = 1, uint32_t z_segments = 1 );

private:
    MeshDesc build_mesh_desc_( uint32_t x_segments, uint32_t z_segments );
};

// ------------------------------------------------------------------------------
// Image, CubeMap
// ------------------------------------------------------------------------------

/**
 * @brief Image contains RGBA pixel data.
 */
class NCAPI Image : public IResource {
    NCLASS( Image, IResource )

public:
    Image() = default;
    Image( int w, int h, const void* rgba_pixels );

    ResourceFormatID get_format_id() const override;
    size_t get_size_bytes() const override;
    uint32_t get_width() const;
    uint32_t get_height() const;
    Span<const std::byte> get_pixels() const [[clang::lifetimebound]];
    void* get_raw() [[clang::lifetimebound]];

    void set_dimension( int w, int h );
    void set_data( const void* rgba_pixels );

private:
    REFLECT int width;
    REFLECT int height;
    REFLECT BytesBuffer pixels;
};

/**
 * @brief CubeMap represents cube-mapped image.
 */
class NCAPI CubeMap : public IResource {
    NCLASS( CubeMap, IResource )

public:
    /**
     * @brief Convert an equirectangular image into 6 cube face images.
     *
     * Assumes the source image's top row corresponds to zenith (+Y) and the
     * horizontal center (u = 0.5) corresponds to the +X direction.
     *
     * Face order: +X, -X, +Y, -Y, +Z, -Z (Vulkan/Diligent cubemap order).
     * Output faces are RGBA8, bilinearly sampled from the source.
     *
     * @param equirect  Source equirectangular image (2:1 aspect recommended).
     * @param face_size Output face resolution (width == height).
     */
    CubeMap( const Ref<Image>& equirect, uint32_t face_size = 0 );

    ResourceFormatID get_format_id() const override;
    size_t get_size_bytes() const override;
    Span<const Ref<Image>, 6> get_faces() const [[clang::lifetimebound]];

private:
    REFLECT Array<Ref<Image>, 6> faces;
};

// ------------------------------------------------------------------------------
// AudioClip
// ------------------------------------------------------------------------------

class NCAPI AudioClip : public IResource {
    NCLASS( AudioClip, IResource )

public:
    AudioClip() = default;
    AudioClip( const void* p_data, int p_length, int p_channels, int p_frequency, int p_bits_per_sample );

    ResourceFormatID get_format_id() const override;
    Span<std::byte> get_data() [[clang::lifetimebound]];
    Span<const std::byte> get_data() const [[clang::lifetimebound]];

    int get_length() const;
    int get_channels() const;
    int get_frequency() const;
    int get_bits_per_sample() const;

    size_t get_size_bytes() const override;

private:
    REFLECT int length          = 0;
    REFLECT int channels        = 0;
    REFLECT int frequency       = 0;
    REFLECT int bits_per_sample = 0;
    REFLECT BytesBuffer data;
};

// ------------------------------------------------------------------------------
// Font
// ------------------------------------------------------------------------------

class NCAPI Font : public IResource {
    NCLASS( Font, IResource )

public:
    Font() = default;

    ResourceFormatID get_format_id() const override;
    size_t get_size_bytes() const override;
    Span<const std::byte> get_data() const [[clang::lifetimebound]];

    void set_data( const void* p_data, size_t p_size );

private:
    REFLECT BytesBuffer data;
};

} // namespace nc
