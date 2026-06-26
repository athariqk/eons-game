#include "SpeciesGuiSystem.h"

#include <ncore/modules/ecs/ecs_world.h>
#include <ncore/modules/video/render_service.h>
// #include <ncore/modules/ecs/ecs_input_system.h>

#include <microcosmos/GameGroups.h>
#include <microcosmos/OrganismFactory.h>
#include <microcosmos/SpeciesRegistry.h>
#include <microcosmos/components/FoodComponent.h>
#include <microcosmos/components/OrganismAIComponent.h>
#include <microcosmos/components/OrganismComponent.h>
#include <microcosmos/components/SpeciesComponent.h>

// void SpeciesGuiSystem::on_init(ncore::EcsWorld &world) {
// reg_ = world.try_context<SpeciesRegistry>();
// return reg_ != nullptr;
//}
//
// void SpeciesGuiSystem::on_gui_render(ncore::EcsWorld &world) {
// if (!reg_)
//    return;

// ImGui::Begin("Microcosmos", nullptr, ImGuiWindowFlags_AlwaysAutoResize);

// draw_species_panel(world);

// static ImVec4 bg_color(114.0f / 255.0f, 144.0f / 255.0f, 154.0f / 255.0f, 200.0f / 255.0f);
// if (ImGui::TreeNode("Background Color")) {
//     ImGui::ColorEdit4("BGColor", reinterpret_cast<float *>(&bg_color));
//     ImGui::TreePop();
// }

// ImGui::Separator();

// if (ImGui::Button("Add species", ImVec2(120, 25)))
//     ImGui::OpenPopup("Create a new species");

// draw_create_species_modal(world);

// ImGui::End();
//}
//
// void SpeciesGuiSystem::draw_species_panel(ncore::EcsWorld &world) {
// auto &all_species = reg_->all_species();
// if (ImGui::TreeNode("SpeciesList", "Species alive: %zu", all_species.size())) {
//    for (int n = 0; n < static_cast<int>(all_species.size()); n++) {
//        ImGui::PushID(n);

//        auto *species = all_species[n];

//        if (ImGui::TreeNode("SpeciesNode", "%s", species->get_name_formatted(true).c_str())) {
//            if (species->entity)
//                ImGui::BulletText("ID: %llu", species->entity->id);
//            ImGui::BulletText("Population: %i", species->population_count);

//            ImGui::TreePop();
//        }

//        auto &all_organisms = reg_->all_organisms();
//        if (ImGui::TreeNode("OrganismList", "Individuals")) {
//            for (int i = 0; i < static_cast<int>(all_organisms.size()); i++) {
//                ImGui::PushID(i);
//                auto *organism = all_organisms[i];
//                if (ImGui::TreeNode("individuals", "Organism %llu", organism->entity ? organism->entity->id : 0)) {
//                    std::string behaviour = "No AI";
//                    if (organism->entity && world.has<OrganismAIComponent>(*organism->entity)) {
//                        auto &ai = world.get<OrganismAIComponent>(*organism->entity);
//                        behaviour = ai.get_current_behavior();
//                    }

//                    ncore::Vec2 vel;
//                    if (organism->entity && world.has<ncore::RigidbodyComponent>(*organism->entity)) {
//                        auto &body = world.get<ncore::RigidbodyComponent>(*organism->entity);
//                        vel = body.velocity;
//                    }

//                    ImGui::Text("Behaviour: %s\nEnergy: %.0f/%.0f\nSpeed: %f\nSize: %f\nAggressiveness: "
//                                "%f\nFitness: %.0f\nMagnitude: %.0f",
//                                behaviour.c_str(), organism->cur_energy, organism->genome.energy_capacity,
//                                organism->genome.speed, organism->genome.size, organism->genome.aggresiveness,
//                                organism->fitness, vel.length());
//                    ImGui::TreePop();
//                }
//                ImGui::PopID();
//            }
//            ImGui::TreePop();
//        }

//        if (ImGui::Button("Add Organism", ImVec2(100, 25)))
//            OrganismFactory::create(world, *reg_, species);

//        ImGui::SameLine();

//        if (ImGui::Button("Make Extinct", ImVec2(100, 25))) {
//            if (species->entity)
//                world.destroy(*species->entity);
//            reg_->untrack_species(species);
//        }

//        ImGui::PopID();
//    }
//    ImGui::TreePop();
//}

// auto &nutrients = world.get_group(GameGroups::NUTRIENTS_GROUP);
// if (ImGui::TreeNode("NutrientList", "Nutrients available: %zu", nutrients.size())) {
//     for (int n = 0; n < static_cast<int>(nutrients.size()); n++) {
//         if (world.has<FoodComponent>(*nutrients[n]))
//             ImGui::Text("Nutrient instance %.0f", world.get<FoodComponent>(*nutrients[n]).cur_energy);
//     }
//     ImGui::TreePop();
// }
//}

// void SpeciesGuiSystem::draw_create_species_modal(ncore::EcsWorld &world) {
// if (ImGui::BeginPopupModal("Create a new species", nullptr, ImGuiWindowFlags_AlwaysAutoResize)) {
//     ImGui::Text("Enter the species' name");
//     ImGui::InputText("Genus", input_genus_, 255);
//     ImGui::InputText("Epithet", input_epithet_, 255);
//     ImGui::Separator();

//    if (ImGui::Button("Create", ImVec2(80, 25))) {
//        if (input_genus_[0] != '\0' && input_epithet_[0] != '\0') {
//            auto &entity = world.create_entity();
//            auto &species = world.emplace<SpeciesComponent>(entity, input_genus_, input_genus_, input_epithet_);
//            species.entity = &entity;
//            reg_->track_species(species);
//            OrganismFactory::create(world, *reg_, &species);
//            input_genus_[0] = '\0';
//            input_epithet_[0] = '\0';
//            ImGui::CloseCurrentPopup();
//        }
//    }

//    ImGui::SameLine();
//    if (ImGui::Button("Cancel", ImVec2(80, 25))) {
//        input_genus_[0] = '\0';
//        input_epithet_[0] = '\0';
//        ImGui::CloseCurrentPopup();
//    }

//    ImGui::EndPopup();
//}
//}
