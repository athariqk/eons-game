#pragma once

#include <ncore.h>

namespace nc {

class Scene;

/**
 * @brief **MUST** be called before any other scene plugins.
 */
void NCAPI register_core_plugin( Scene& scene );
void NCAPI register_video_plugin( Scene& scene );
void NCAPI register_inputs_plugin( Scene& scene );
// void NCAPI register_gui_plugin( Scene& scene );
void NCAPI register_audio_plugin( Scene& scene );
void NCAPI register_debug_plugin( Scene& scene );

//------------------------------------------------------------------------------

// void NCAPI unregister_gui_plugin( Scene& scene );

} // namespace nc
