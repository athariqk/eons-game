#pragma once

#include <cstdint>
#include <functional>
#include <string>

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
 * @brief For reference see https://github.com/DiligentGraphics/DiligentFX/blob/master/PBR/interface/PBR_Renderer.hpp
 */
class RendererStorage {
public:
    enum PSOFlags : uint64_t {
        NONE                   = 0,
        PSO_CULL_SHIFT         = 0,
        PSO_CULL_MASK          = 3 << PSO_CULL_SHIFT,
        PSO_DEPTH_TEST         = 1 << 2,
        PSO_DEPTH_WRITE        = 1 << 3,
        PSO_BLEND_SHIFT        = 4,
        PSO_BLEND_MASK         = 15 << PSO_BLEND_SHIFT,
        PSO_TOPOLOGY_SHIFT     = 8,
        PSO_TOPOLOGY_MASK      = 15 << PSO_TOPOLOGY_SHIFT,
        PSO_RT_FMT_SHIFT       = 12,
        PSO_RT_FMT_MASK        = 127 << PSO_RT_FMT_SHIFT,
        PSO_MSAA_COUNT_SHIFT   = 19,
        PSO_MSAA_COUNT_MASK    = 15 << PSO_MSAA_COUNT_SHIFT,
        PSO_MSAA_QUALITY_SHIFT = 23,
        PSO_MSAA_QUALITY_MASK  = 15 << PSO_MSAA_QUALITY_SHIFT,
        PSO_SCISSOR            = 1 << 27,
        PSO_FILL_SHIFT         = 28,
        PSO_FILL_MASK          = 3 << PSO_FILL_SHIFT,
    };

    struct PSOKey {
        PSOFlags flags   = PSOFlags::NONE;
        const Shader* vs = nullptr;
        const Shader* ps = nullptr;
        VertexLayout vertex_layout;
        DynamicArray<RID> resource_signatures;
        std::string debug_name;

        bool operator==( const PSOKey& o ) const;
    };

    struct PSOKeyHasher {
        std::size_t operator()( const PSOKey& p ) const;
    };

    struct Material {
        RID pso;
        DynamicArray<RID> resource_signatures;
        DynamicArray<RID> srbs;
        RID sampler;
        RID constant_buffer;

        struct TextureSlot {
            std::string name;
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
        Mat4 CameraMatrix;
        Mat4 ViewProjMatrix; // This is Projection * View
        float Time;
        float DeltaTime;
    };

    void set_rhi( IRHI* p_rhi )
    {
        rhi = p_rhi;
    }

    RID get_pipeline_or_create( const PSOKey& key );

    RID material_create( const MaterialTemplate& tmpl );
    void material_set_texture( RID handle, RID texture, uint32_t slot );
    void material_bind( RID handle, const ShaderConstants& constants );

    RID gpu_mesh_create( const Mesh& mesh );
    void gpu_mesh_bind( RID handle );
    GPUMesh* get_gpu_mesh( RID handle );

    void destroy_rid( RID rid );
    void flush_pending_destroys();

private:
    Material* get_material_( RID handle );
    PSOKey get_pso_key_( const MaterialTemplate& tmpl );
    DynamicArray<ResourceSignatureDesc> build_resource_signatures_( const MaterialTemplate& tmpl );

    IRHI* rhi;
    HashMap<PSOKey, RID, PSOKeyHasher> pso_cache;
    ResourcePool<Material> materials;
    ResourcePool<GPUMesh> gpu_meshes;
    DynamicArray<RID> pending_destroys;
};

} // namespace nc
