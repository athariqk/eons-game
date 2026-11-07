// Copyright (C) 2026 Ahmad Ghalib Athariq <alib.athariq@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.
// File: main_scene.h

#pragma once

#include <ncore/runtime/resources/cubemap.h>
#include <ncore/runtime/scene.h>

namespace sea {

class MainScene : public nc::Scene {
public:
    void on_ready() override;
    void on_exit() override;

private:
    void create_environment();
    void create_water( nc::Node* camera );

    bool editor_mode = false;
    nc::Ref<nc::CubeMap> skybox_cubemap;
};

} // namespace sea
