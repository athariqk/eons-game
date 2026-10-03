#pragma once

#include <ncore/core/types.h>
#include <ncore/runtime/ecs/ecs_component.h>
#include <ncore/runtime/resources/resource_loader.h>

namespace nc {

class Scene;

NC_COMPONENT_API( ResourceWatchState )
{
    REFLECT DynamicArray<ResourceLoader::Event> PendingEvents;
};

/**
 * @brief **Must** be called before any other scene plugins.
 */
void NCAPI register_core_plugin( Scene& scene );
void NCAPI register_video_plugin( Scene& scene );
void NCAPI register_inputs_plugin( Scene& scene );
// void NCAPI register_gui_plugin( Scene& scene );
void NCAPI register_audio_plugin( Scene& scene );
void NCAPI register_resources_plugin( Scene& scene );
void NCAPI register_debug_plugin( Scene& scene );

//------------------------------------------------------------------------------

// void NCAPI unregister_gui_plugin( Scene& scene );

} // namespace nc
