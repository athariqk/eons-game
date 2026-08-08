#pragma once

#include <ncore/core/rid.h>
#include <ncore/core/types.h>
#include <ncore/core/vector.h>

namespace nc {

class Scene;

struct NCAPI RenderState {
    Vec2 display_size{};
    RID white_texture;
    NSTRUCT( RenderState, NC_F( RenderState, display_size ) )
};

void NCAPI register_window_plugin( Scene& scene );
void NCAPI register_render_plugin( Scene& scene );
void NCAPI register_inputs_plugin( Scene& scene );
void NCAPI register_gui_plugin( Scene& scene );
void NCAPI register_audio_plugin( Scene& scene );
void NCAPI register_physics_plugin( Scene& scene );

} // namespace nc
