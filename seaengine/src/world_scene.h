#pragma once

#include <ncore/runtime/node.h>
#include <ncore/runtime/scene.h>

namespace sea {

class WorldScene : public nc::Scene {
public:
    void on_ready() override;
    void on_exit() override;

private:
    void create_environment();
    void create_water( nc::Node* camera );

    nc::RID skybox_tex;
};

} // namespace sea
