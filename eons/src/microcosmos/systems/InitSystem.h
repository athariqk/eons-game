#pragma once

#include <ncore/runtime/ecs/ecs_system.h>

class InitSystem : public ncore::EcsSystem {
public:
    InitSystem() { set_priority(-1000); }

    void on_init(ncore::EcsWorld &world) override;

private:
    bool has_run_ = false;
};
