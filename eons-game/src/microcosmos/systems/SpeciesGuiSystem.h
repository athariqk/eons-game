#pragma once

#include <ncore/runtime/ecs_system.h>

// class SpeciesRegistry;
//
// class SpeciesGuiSystem : public nc::EcsSystem {
//     NCLASS(SpeciesGuiSystem, nc::EcsSystem)
//
// public:
//     SpeciesGuiSystem() { set_priority(1000); }
//
//     void on_init(nc::EcsWorld &world) override;
//     void on_gui_render(nc::EcsWorld &world) override;
//
// private:
//     void draw_species_panel(nc::EcsWorld &world);
//     void draw_create_species_modal(nc::EcsWorld &world);
//
//     SpeciesRegistry *reg_ = nullptr;
//     char input_genus_[255] = {};
//     char input_epithet_[255] = {};
// };
