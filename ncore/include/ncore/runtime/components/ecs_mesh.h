#pragma once

#include <ncore/core/reference.h>
#include <ncore/core/rid.h>
#include <ncore/resources/mesh.h>

namespace nc {

struct NCAPI EcsMeshInstance {
    Ref<Mesh> mesh_resource;
    RID gpu_mesh = 0;

    NSTRUCT( EcsMeshInstance, NC_F( EcsMeshInstance, mesh_resource ) NC_F( EcsMeshInstance, gpu_mesh ) )
};

} // namespace nc
