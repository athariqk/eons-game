#pragma once

#include <cstdint>
#include <functional>
#include <string>

#include <ncore/core/rid.h>

#include "../rhi_types.h"
#include "material.h"

namespace nc {

class IRHI;
class Shader;
class MaterialTemplate;
struct ShaderParamInfo;
using ShaderParamLayout = Vector<ShaderParamInfo>;

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
    };

    struct PSOKey {
        PSOFlags flags   = PSOFlags::NONE;
        const Shader* vs = nullptr;
        const Shader* ps = nullptr;
        VertexLayout vertex_layout;
        Vector<RID> resource_signatures;
        std::string debug_name;

        bool operator==( const PSOKey& o ) const;
    };

    struct PSOKeyHasher {
        std::size_t operator()( const PSOKey& p ) const;
    };

    RID get_pipeline_or_create( const PSOKey& key, IRHI* rhi );

    RID material_create( const MaterialTemplate& tmpl, IRHI* rhi );
    Material* get_material( RID handle );
    void destroy_materials();

private:
    PSOKey get_pso_key_( const MaterialTemplate& tmpl );
    Vector<ResourceSignatureDesc> build_resource_signatures_( const MaterialTemplate& tmpl );

    HashMap<PSOKey, RID, PSOKeyHasher> pso_cache;
    ResourcePool<Material> materials;
};

} // namespace nc
