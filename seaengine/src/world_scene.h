#pragma once

#include <ncore/core/rid.h>
#include <ncore/runtime/scene.h>

namespace sea {

class WorldScene : public nc::Scene {
public:
    void on_ready() override;
    void on_exit() override;

private:
    void create_environment();
    void create_water();

    nc::RID skybox_cubemap_rid_ = 0;
};

} // namespace sea
