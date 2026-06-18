#pragma once

#include <ncore.h>

class MicrocosmModule : public ncore::EcsModule {
    NCLASS(MicrocosmModule, ncore::EcsModule)

public:
    void build(ncore::EcsWorld &world) override;
};
