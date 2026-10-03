#pragma once

#include <cstdint>

#include <ncore/core/matrix.h>
#include <ncore/core/reference.h>
#include <ncore/resources/resource.h>

#include "../gfx_types.h"

namespace nc {

class GfxInterface;
class Mesh;

/**
 * @brief RenderStorage is responsible for managing RenderService
 * resources such as material variants, shader resources and meshes.
 *
 * For reference see https://github.com/DiligentGraphics/DiligentFX/blob/master/PBR/interface/PBR_Renderer.hpp
 */
class RenderStorage {
public:
    /**
     * @brief A tightly-packed bitfield for encoding PSO permutations.
     */
    enum PSOFlags : uint64_t {
        NONE                   = 0,
        PSO_CULL_SHIFT         = 0,
        PSO_CULL_MASK          = 3 << PSO_CULL_SHIFT,          // enum, 3-value range
        PSO_DEPTH_TEST         = 1 << 2,                       // bool
        PSO_DEPTH_WRITE        = 1 << 3,                       // bool
        PSO_BLEND_SHIFT        = 4,
        PSO_BLEND_MASK         = 15 << PSO_BLEND_SHIFT,        // enum, 15-value range
        PSO_TOPOLOGY_SHIFT     = 8,
        PSO_TOPOLOGY_MASK      = 15 << PSO_TOPOLOGY_SHIFT,     // enum, 15-value range
        PSO_RT_FMT_SHIFT       = 12,
        PSO_RT_FMT_MASK        = 7 << PSO_RT_FMT_SHIFT,        // enum, 7-value range
        PSO_DST_FMT_SHIFT      = 15,
        PSO_DST_FMT_MASK       = 15 << PSO_DST_FMT_SHIFT,      // enum, 15-value range
        PSO_MSAA_COUNT_SHIFT   = 19,
        PSO_MSAA_COUNT_MASK    = 15 << PSO_MSAA_COUNT_SHIFT,   // enum, 15-value range
        PSO_MSAA_QUALITY_SHIFT = 23,
        PSO_MSAA_QUALITY_MASK  = 15 << PSO_MSAA_QUALITY_SHIFT, // enum, 15-value range
        PSO_SCISSOR            = 1 << 27,                      // bool
        PSO_FILL_SHIFT         = 28,
        PSO_FILL_MASK          = 3 << PSO_FILL_SHIFT,
    };

    /**
     * @brief POD of literally a key to a PSO.
     * Contains PSO traits and can be used to define one.
     */
    struct PSOKey {
        PSOFlags flags = PSOFlags::NONE;
        RID vs; // RHI-level vert shader obj.
        RID ps; // RHI-level pixel shader obj.
        RID cs; // RHI-level compute shader obj.
        gfx::VertexLayout vertex_layout;
        DynamicArray<RID> res_signatures;
        String debug_name;

        bool operator==( const PSOKey& o ) const;
    };

    struct PSOKeyHasher {
        std::size_t operator()( const PSOKey& p ) const;
    };

    static constexpr gfx::BindingSet BINDING_SET_SHADER_GLOBAL = 0;
    static constexpr gfx::BindingSet BINDING_SET_MATERIAL      = 1;
    static constexpr gfx::BindingSet BINDING_SET_SCENE         = 2;
    static constexpr gfx::BindingSet BINDING_SET_INSTANCE      = 3;

    /**
     * @brief Size of the spatial [[vk::push_constant]] block, in 4-byte constants.
     *
     * Must match `ConstantBuffer<SpatialInstance>` in shaders/ncore/spatial.slang and
     * RenderService::SpatialInstanceData. Diligent derives the VkPushConstantRange from
     * PipelineResourceDesc::ArraySize (for inline constants it is a count of 4-byte values),
     * so an undersized value makes Vulkan reject the PSO with
     * VUID-VkGraphicsPipelineCreateInfo-layout-10069 and makes SetInlineConstants() assert.
     */
    static constexpr uint32_t SPATIAL_PUSH_CONSTANT_UINTS = 32; // 128 bytes

    /**
     * @brief Size of the canvas [[vk::push_constant]] block, in 4-byte constants.
     *
     * Must match `ConstantBuffer<CanvasInstance>` in shaders/ncore/canvas.slang and
     * RenderService::CanvasInstanceData (a single Mat4).
     */
    static constexpr uint32_t CANVAS_PUSH_CONSTANT_UINTS = 16; // 64 bytes

    /**
     * @brief Create binding and mapping of resources on the given binding set for a shader.
     */
    RID resource_set_create(
        const Shader& p_shader, gfx::BindingSet p_set, Span<const gfx::ResourceMappingEntry> p_resources
    );
    /**
     * @brief Get the resource signature RID for a given resource set.
     *
     * Used to include additional signatures in PSO creation.
     */
    RID resource_set_get_signature( RID p_resource_set );
    /**
     * @brief Commit this resource set to the device context.
     */
    void resource_set_bind( RID p_resource_set );

    RID get_render_pipeline_or_create( const PSOKey& key );
    /**
     * @brief Create-or-get a cached compute pipeline.
     */
    RID get_compute_pipeline_or_create( const PSOKey& key );

    struct SceneData {
        uint32_t render_width;
        uint32_t render_height;
        // std140 aligns the following mat4 to a 16-byte boundary; Mat4 itself is
        // 4-byte aligned, so the padding has to be explicit or the GPU layout
        // (CameraMatrix @16, ViewProjMatrix @80, Time @144) diverges from this one.
        uint32_t std140_pad0[2];
        // The current camera's transform.
        Mat4 camera_matrix;
        // The current camera's inverse matrix pre-multiplied with the projection matrix.
        Mat4 view_proj_matrix;
        // Elapsed time in seconds.
        float time;
        // Elapsed time between frames.
        float delta_time;
        // Near/far clipping planes of the current camera.
        float z_near;
        float z_far;
    };

    /**
     * @brief Update, upload and bind the scene data buffer to the GPU.
     */
    void scene_data_update( const SceneData& p_scene_data );

    /**
     * @brief Create GPU-side material instance from Material resource.
     *
     * A material has its own shader resources.
     */
    RID material_create( const Ref<MaterialShader>& p_shader );
    void material_set_param( RID p_material, const String& p_param, const void* p_data, size_t p_data_size );
    void material_set_texture( RID p_material, RID p_texture, uint32_t p_slot );
    void material_set_draw_mode( RID p_material, gfx::FillMode p_mode );
    /**
     * @brief Bind material instance to the current render pipeline.
     */
    void material_bind( RID p_material, bool p_pass_has_depth );
    bool material_has_push_constants( RID p_material );

    /**
     * @brief Get-or-create a material variant whose texture slot 0 is bound to `p_texture`.
     *
     * Avoids mutating the shared base material during the draw loop: the variant is
     * created once per (material, texture) pair and cached for reuse across frames.
     */
    RID material_create_texture_variant( RID p_material, RID p_texture );

    /**
     * @brief Creates vertex/index buffer objects from Mesh resource.
     *
     * Currently the access mode for the buffers is immutable, so we
     * can't update GPU-side meshes dynamically.
     */
    RID mesh_create( const Mesh& mesh );
    /**
     * @brief Bind GPU mesh to the current render pipeline.
     */
    void mesh_bind( RID handle );
    uint32_t mesh_get_index_count( RID p_gpu_mesh );

    bool is_rid_owned( RID rid );
    bool destroy_rid( RID rid );
    /**
     * @brief Commit all deferred destroy_rid(RID) calls.
     */
    void flush_pending_destroys();

    /**
     * @brief Sets current Graphics API implementation. Called automatically in RenderService init.
     */
    void set_graphics_api( GfxInterface* p_gfx_api );

    /**
     * @brief Advance the per-frame counter used to decide when a dynamic buffer must be
     * re-mapped. Diligent invalidates dynamic allocations at the end of every frame, so
     * "written once" is only valid for the frame the write happened in.
     */
    void next_frame()
    {
        ++frame_index;
    }

private:
    friend class RenderService;
    uint64_t frame_index = 0;

    /**
     * @brief GPU-side data of a Material.
     */
    struct MaterialData {
        Ref<MaterialShader> shader; // kept for runtime reflection (set_param, set_texture)
        RID pso;
        PSOKey pso_key;
        // Same permutation but declaring no depth attachment (DSVFormat = UNKNOWN). A material may be
        // drawn into passes with and without a bound depth buffer in the same frame, and Vulkan's
        // dynamic-rendering rules require the pipeline's declared depth format to match the render pass.
        RID pso_flat;
        PSOKey pso_key_flat;
        RID material_res_set;            // resource set for BINDING_SET_MATERIAL
        bool has_push_constants = false; // shader declares a [[vk::push_constant]] block
        bool has_light_env      = false; // shader references g_LightEnv in set 0
        RID param_buffer;                // Per-material constant buffer
        DynamicArray<uint8_t> param_staging;
        bool param_dirty = false;
        // Diligent discards dynamic allocations at the end of every frame, so a buffer that
        // is drawn to must be re-mapped in each frame. Tracks the last frame it was written.
        uint64_t param_frame = ~0ull;
        struct TextureSlot {
            gfx::ParameterType type;
            RID texture_rid;
        };
        HashMap<String, TextureSlot> texture_slots;
        DynamicArray<String> texture_slot_names; // ordered: slot index → name
    };

    struct ResourceSet {
        RID signature;
        RID mapping;
        RID binding;
        uint16_t set = 0;
        DynamicArray<gfx::ResourceType> slot_kinds;
    };

    struct ResSignatureKey {
        const Shader* shader;
        uint16_t set;

        bool operator==( const ResSignatureKey& o ) const;
    };
    struct ResSignatureKeyHasher {
        std::size_t operator()( const ResSignatureKey& p ) const;
    };

    struct TextureVariantKey {
        RID material;
        RID texture;

        bool operator==( const TextureVariantKey& o ) const;
    };
    struct TextureVariantKeyHasher {
        std::size_t operator()( const TextureVariantKey& p ) const;
    };

    /**
     * @brief GPU-side Mesh resource.
     */
    struct GPUMesh {
        RID vertices;
        RID indices;
        uint32_t index_count;
    };

private:
    /**
     * @brief Bake a Material resource into PSO key.
     */
    PSOKey get_pso_key_( const Ref<MaterialShader>& p_shader );

    GfxInterface* gfx_api;
    HashMap<PSOKey, RID, PSOKeyHasher> pso_cache;
    HashMap<ResSignatureKey, RID, ResSignatureKeyHasher> res_signature_cache;
    HashMap<TextureVariantKey, RID, TextureVariantKeyHasher> texture_variant_cache;
    RIDPool<ResourceSet> resource_sets;
    RIDPool<MaterialData> materials;
    RIDPool<GPUMesh> meshes;
    HashSet<RID> pending_destroys;

    RID scene_data_ubo;
    RID scene_data_ubo_res_set;
    void ensure_scene_data_ubo_();

    // `g_LightEnv` lives in BINDING_SET_SHADER_GLOBAL (set 0) of every spatial
    // shader. The engine has no light system yet, so a single default "sun" is
    // kept in a staging block — a zero-filled block makes every surface black.
    RID light_env_ubo;
    RID light_env_res_set;
    uint64_t light_env_frame = ~0ull;
    DynamicArray<uint8_t> light_env_data_;
    void ensure_light_env_();

    RID spatial_push_sig;
    RID spatial_push_binding;
    RID canvas_push_sig;
    RID canvas_push_binding;
    void ensure_push_constant_signatures_();

    RID white_texture_2d;
    RID white_texture_2d_array;
    RID white_texture_cube;
    RID get_fallback_texture_( gfx::ParameterType type );

    // material signatures also contain SamplerState slots, which nothing else in
    // the engine binds — every material gets this one until a real sampler is set
    RID default_sampler;
    RID ensure_default_sampler_();
    void ensure_fallback_textures_();

    uint32_t align_up_( uint32_t p_value, uint32_t p_alignment );
};

} // namespace nc
