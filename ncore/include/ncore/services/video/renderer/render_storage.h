#pragma once

#include <cstdint>

#include <ncore/core/matrix.h>
#include <ncore/core/rid.h>

#include "../rhi_types.h"

namespace nc {

class IRHI;
class Shader;
class MaterialTemplate;
class Mesh;
struct ShaderParamInfo;
using ShaderParamLayout = DynamicArray<ShaderParamInfo>;

/**
 * @brief RenderStorage is responsible for managing RenderService
 * resources such as material variants, shaders and meshes.
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
        PSO_DST_FMT_MASK       = 15 << PSO_RT_FMT_SHIFT,       // enum, 15-value range
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
        PSOFlags flags   = PSOFlags::NONE;
        const Shader* vs = nullptr;
        const Shader* ps = nullptr;
        VertexLayout vertex_layout;
        DynamicArray<RID> res_signatures;
        String debug_name;

        bool operator==( const PSOKey& o ) const;
    };

    struct PSOKeyHasher {
        std::size_t operator()( const PSOKey& p ) const;
    };

    /**
     * @brief Instance of a MaterialTemplate.
     *
     * This is our implementation of the material system concept
     * typically found in games/engines.
     */
    struct Material {
        RID pso;
        PSOKey pso_key;
        DynamicArray<RID> res_signatures;
        DynamicArray<RID> srbs;
        RID sampler;
        RID constant_buffer;

        struct TextureSlot {
            String name;
            size_t srb_index;
        };
        DynamicArray<TextureSlot> texture_slots;
    };

    struct GPUMesh {
        RID vertices;
        RID indices;
        uint32_t index_count;
    };

    struct ShaderConstants {
        Mat4 ModelMatrix;
        Mat4 ModelMatrixInv;
        Mat4 CameraMatrix;
        Mat4 ViewProjMatrix; // This is Projection * View
        float Time;
        float DeltaTime;
    };

    void set_graphics_api( IRHI* p_gfx_api );

    RID get_pipeline_or_create( const PSOKey& key );

    /**
     * @brief Create GPU-side material instance from MaterialTemplate.
     */
    RID material_create( const MaterialTemplate& tmpl );
    void material_set_texture( RID handle, RID texture, uint32_t slot );
    void material_set_draw_mode( RID handle, FillMode mode );
    /**
     * @brief Bind material instance to the current gfx pipeline.
     */
    void material_bind( RID handle, const ShaderConstants& constants );

    /**
     * @brief Creates vertex/index buffer objects from Mesh resource.
     *
     * Currently the access mode for the buffers is immutable, so we
     * can't update GPU-side meshes dynamically.
     */
    RID gpu_mesh_create( const Mesh& mesh );
    /**
     * @brief Bind GPU mesh to the current gfx pipeline.
     */
    void gpu_mesh_bind( RID handle );
    GPUMesh* get_gpu_mesh( RID handle );

    bool is_rid_owned( RID rid );
    bool destroy_rid( RID rid );
    /**
     * @brief Commit all deferred destroy_rid(RID) calls.
     */
    void flush_pending_destroys();

private:
    void ensure_fallback_texture_();
    Material* get_material_( RID handle );
    PSOKey get_pso_key_( const MaterialTemplate& tmpl );
    DynamicArray<ResourceSignatureDesc> build_resource_signatures_( const MaterialTemplate& tmpl );

    IRHI* gfx_api;
    HashMap<PSOKey, RID, PSOKeyHasher> pso_cache;
    RIDPool<Material> materials;
    RIDPool<GPUMesh> gpu_meshes;
    DynamicArray<RID> pending_destroys;
    RID white_texture;
};

} // namespace nc
