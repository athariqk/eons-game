#pragma once

#include <ncore/modules/ecs/ecs_system.h>

// class SpeciesRegistry;
//
// class SpeciesGuiSystem : public ncore::EcsSystem {
//     NCLASS(SpeciesGuiSystem, ncore::EcsSystem)
//
// public:
//     SpeciesGuiSystem() { set_priority(1000); }
//
//     void on_init(ncore::EcsWorld &world) override;
//     void on_gui_render(ncore::EcsWorld &world) override;
//
// private:
//     void draw_species_panel(ncore::EcsWorld &world);
//     void draw_create_species_modal(ncore::EcsWorld &world);
//
//     SpeciesRegistry *reg_ = nullptr;
//     char input_genus_[255] = {};
//     char input_epithet_[255] = {};
// };
