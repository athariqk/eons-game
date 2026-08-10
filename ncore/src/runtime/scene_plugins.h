#pragma once

#include <ncore/core/rid.h>
#include <ncore/core/types.h>
#include <ncore/core/vector.h>
#include <ncore/services/io/resource_service.h>

namespace nc {

class Scene;

struct NCAPI RenderState {
    Vec2 display_size{};
    RID white_texture;
    NSTRUCT( RenderState, NC_F( RenderState, display_size ) )
};

struct NCAPI ResourceWatchState {
    DynamicArray<ResourceService::Event> pending_events;
    NSTRUCT( ResourceWatchState, NC_F( ResourceWatchState, pending_events ) )
};

/**
 * @brief **Must** be called before any other scene plugins.
 */
void NCAPI register_core_plugin( Scene& scene );
void NCAPI register_window_plugin( Scene& scene );
void NCAPI register_render_plugin( Scene& scene );
void NCAPI register_inputs_plugin( Scene& scene );
void NCAPI register_gui_plugin( Scene& scene );
void NCAPI register_audio_plugin( Scene& scene );
void NCAPI register_physics_plugin( Scene& scene );
void NCAPI register_resources_plugin( Scene& scene );

} // namespace nc
