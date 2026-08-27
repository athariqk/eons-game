#pragma once

#include <ncore/core/types.h>
#include <ncore/services/io/resource_service.h>

namespace nc {

class Scene;

struct NCAPI ResourceWatchState {
    DynamicArray<ResourceService::Event> PendingEvents;
    NSTRUCTV( ResourceWatchState, NC_F( ResourceWatchState, PendingEvents ) )
};

/**
 * @brief **Must** be called before any other scene plugins.
 */
void NCAPI register_core_plugin( Scene& scene );
void NCAPI register_video_plugin( Scene& scene );
void NCAPI register_inputs_plugin( Scene& scene );
// void NCAPI register_gui_plugin( Scene& scene );
void NCAPI register_audio_plugin( Scene& scene );
void NCAPI register_physics_plugin( Scene& scene );
void NCAPI register_resources_plugin( Scene& scene );
void NCAPI register_debug_plugin( Scene& scene );

//------------------------------------------------------------------------------

// void NCAPI unregister_gui_plugin( Scene& scene );

} // namespace nc
