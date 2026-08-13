#pragma once

#include <ncore/runtime/scene.h>

namespace sea {

class WorldScene : public nc::Scene {
public:
    void on_ready() override;

private:
    void create_environment();
    void create_water();
};

} // namespace sea
