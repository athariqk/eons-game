#include "MicrocosmWorld.h"

#include <algorithm>
#include <vector>

#include <SDL3/SDL_mouse.h>
#include <imgui.h>

#include <modules/MainLoop.h>
#include <modules/Services.h>
#include <modules/ecs/components/RigidBodyComponent.h>
#include <modules/ecs/components/SpriteComponent.h>
#include <modules/ecs/components/TransformComponent.h>
#include <modules/ecs/systems/AudioSystem.h>
#include <modules/ecs/systems/InputSystem.h>
#include <modules/ecs/systems/RenderSystem.h>
#include <modules/events/InputEvent.h>
#include <modules/graphics/Viewport.h>
#include <modules/graphics/Window.h>
#include <utils/Random.h>

#include "components/FoodComponent.h"
#include "components/OrganismAIComponent.h"
#include "components/OrganismComponent.h"
#include "components/SpeciesComponent.h"
#include "systems/FoodSystem.h"
#include "systems/OrganismAISystem.h"
#include "systems/OrganismSystem.h"
#include "systems/SpeciesSystem.h"

MicrocosmWorld::MicrocosmWorld() : cfg_map(cfg_filename) { set_input_empty(); }

void MicrocosmWorld::register_systems() {
    // Register all game systems
    add_system<SpeciesSystem>();
    add_system<OrganismSystem>();
    add_system<FoodSystem>();
    add_system<OrganismAISystem>();
}

void MicrocosmWorld::on_init() {
    cfg_cam = cfg_map.load<cfg::MicrocosmCamera>();

    register_systems();

    inputs = get_system<ncore::InputSystem>();
    viewport = get_main_loop().get_services().try_get<ncore::Viewport2D>();
    main_cam = viewport ? viewport->get_main_camera() : nullptr;
    rendering = get_system<ncore::RenderSystem>();

    /* Initial species */
    add_species("Primum", "Primus", "specium");
}

void MicrocosmWorld::on_update(const double p_delta) {
    if (inputs && viewport && main_cam) {
        update_cam_ctrl(p_delta);
        update_cam_movement(p_delta);
    }
}

void MicrocosmWorld::on_gui_render() {
    ImGui::Begin("Microcosmos", nullptr, ImGuiWindowFlags_AlwaysAutoResize);

    if (ImGui::Button("Reset", ImVec2(100, 20))) {
        get_main_loop().change_world(std::make_unique<MicrocosmWorld>());
    }

    if (ImGui::TreeNode("SpeciesList", "Species alive: %i", species_reg.size())) {
        for (int n = 0; n < species_reg.size(); n++) {
            ImGui::PushID(n);

            auto species = species_reg[n];

            if (ImGui::TreeNode("SpeciesNode", "%s", species->get_name_formatted(true).c_str())) {
                ImGui::BulletText("ID: %i", species->entity->get_id());
                ImGui::BulletText("Population: %i", species->population_count);

                ImGui::TreePop();
            }

            if (ImGui::TreeNode("OrganismList", "Individuals")) {
                for (int i = 0; i < organism_reg.size(); i++) {
                    ImGui::PushID(i);
                    if (const auto organism = organism_reg[i];
                        ImGui::TreeNode("individuals", "Organism %i", organism->entity->get_id())) {

                        // Get AI component if it exists
                        std::string behaviour = "No AI";
                        if (organism->entity->has_component<OrganismAIComponent>()) {
                            auto &ai = organism->entity->get_component<OrganismAIComponent>();
                            behaviour = ai.get_current_behavior();
                        }

                        auto &body = organism->entity->get_component<ncore::RigidBodyComponent>();

                        ImGui::Text("Behaviour: %s\nEnergy: %.0f/%.0f\nSpeed: %f\nSize: %f\nAggressiveness: "
                                    "%f\nMembraneColour.r: %i\nMembraneColour.g: %i\nMembraneColour.b: "
                                    "%i\nFitness: %.0f\nMagnitude: %.0f",
                                    behaviour.c_str(), organism->cur_energy, organism->genome.energy_capacity,
                                    organism->genome.speed, organism->genome.size, organism->genome.aggresiveness,
                                    organism->genome.membrane_color.r, organism->genome.membrane_color.g,
                                    organism->genome.membrane_color.b, organism->fitness, body.velocity.length());
                        ImGui::TreePop();
                    }
                    ImGui::PopID();
                }
                ImGui::TreePop();
            }

            if (ImGui::Button("Add Organism", ImVec2(100, 25))) {
                add_organism(species);
            }

            ImGui::SameLine();

            if (ImGui::Button("Make Extinct", ImVec2(100, 25))) {
                remove_species(species);
            }

            ImGui::PopID();
        }
        ImGui::TreePop();
    }

    if (const auto &nutrients(get_group(MicrocosmWorld::NUTRIENTS_GROUP));
        ImGui::TreeNode("NutrientList", "Nutrients available: %i", nutrients.size())) {
        for (int n = 0; n < nutrients.size(); n++) {
            ImGui::Text("Nutrient instance %.0f", nutrients[n]->get_component<FoodComponent>().cur_energy);
        }
        ImGui::TreePop();
    }

    static auto color = ImVec4(114.0f / 255.0f, 144.0f / 255.0f, 154.0f / 255.0f, 200.0f / 255.0f);

    if (ImGui::TreeNode("Background Color")) {
        ImGui::ColorEdit4("BGColor", reinterpret_cast<float *>(&color));
        ImGui::TreePop();
    }

    ImGui::Separator();

    if (ImGui::Button("Add species", ImVec2(120, 25)))
        ImGui::OpenPopup("Create a new species");

    /* ----- Create new species modal|begin| ------- */
    {
        if (ImGui::BeginPopupModal("Create a new species", nullptr, ImGuiWindowFlags_AlwaysAutoResize)) {
            ImGui::Text("Enter the species' name");
            ImGui::InputText("Genus", inputGenus, 255);
            ImGui::InputText("Epithet", inputEpithet, 255);
            ImGui::Separator();
            if (ImGui::Button("Create", ImVec2(80, 25))) {
                if (get_is_inputy_empty(inputGenus) || get_is_inputy_empty(inputEpithet)) {
                    NC_LOG_ERROR("Genus or epithet is not valid!");
                } else {
                    add_species(inputGenus, inputGenus, inputEpithet);
                    set_input_empty();
                    ImGui::CloseCurrentPopup();
                }
            }
            ImGui::SameLine();
            if (ImGui::Button("Random", ImVec2(80, 25))) {
            }
            ImGui::SameLine();
            if (ImGui::Button("Cancel", ImVec2(80, 25))) {
                ImGui::CloseCurrentPopup();
                set_input_empty();
            }

            ImGui::EndPopup();
        }
    } /* ----- Create new species popup|end| ------- */

    ImGui::End();
}

void MicrocosmWorld::on_finish() {}

void MicrocosmWorld::update_cam_ctrl(double p_delta) {
    const float delta = static_cast<float>(p_delta);

    cam_input_dir.zero();

    if (inputs->get_is_key_pressed(ncore::KeyboardEvent::Key::W))
        cam_input_dir.y -= 1.0f;
    if (inputs->get_is_key_pressed(ncore::KeyboardEvent::Key::S))
        cam_input_dir.y += 1.0f;
    if (inputs->get_is_key_pressed(ncore::KeyboardEvent::Key::A))
        cam_input_dir.x -= 1.0f;
    if (inputs->get_is_key_pressed(ncore::KeyboardEvent::Key::D))
        cam_input_dir.x += 1.0f;

    const float lenSq = cam_input_dir.length_sqr();
    if (lenSq > 0.001f)
        cam_input_dir *= 1.0f / std::sqrt(lenSq);

    is_dragging = inputs->get_is_mouse_button_pressed(ncore::ButtonIndex::MIDDLE);

    if (is_dragging) {
        const float zoom = main_cam->get_zoom();
        const float ppm = viewport->get_pixels_per_meter();
        const auto md = inputs->get_last_mouse_delta();

        auto target_vel = (-md / (ppm * zoom)) / delta;
        const float t = 1.0f - std::exp(-cfg_cam.DragSensitivity * delta);
        cam_velocity += (target_vel - cam_velocity) * t;

        // Zero out WASD input so keyboard doesn't interfere
        cam_input_dir.zero();
    }

    cam_input_zoom = inputs->get_last_mouse_wheel_delta().y;
}

void MicrocosmWorld::update_cam_movement(const double p_delta) {
    const float delta = static_cast<float>(p_delta);

    // Movement
    if (!is_dragging) {
        cam_velocity += cam_input_dir * cfg_cam.Acceleration * delta;

        const float friction = 1.0f - std::exp(-cfg_cam.Friction * delta);
        cam_velocity -= cam_velocity * friction;

        const float speedSq = cam_velocity.length_sqr();
        if (speedSq > cfg_cam.MaxSpeed * cfg_cam.MaxSpeed)
            cam_velocity *= cfg_cam.MaxSpeed / std::sqrt(speedSq);
    }

    auto pos = main_cam->get_position();
    pos += cam_velocity * delta;
    main_cam->set_position(pos);

    // Zoom
    cam_velocity_zoom += cam_input_zoom * cfg_cam.ZoomSensitivity;
    const float zoomFriction = 1.0f - std::exp(-cfg_cam.ZoomFriction * delta);
    cam_velocity_zoom -= cam_velocity_zoom * zoomFriction;

    const float newZoom =
        std::clamp(main_cam->get_zoom() + cam_velocity_zoom * delta, cfg_cam.MinZoom, cfg_cam.MaxZoom);

    main_cam->set_zoom(newZoom);
}

void MicrocosmWorld::add_species(const std::string &name, const std::string &genus, const std::string &epithet) {
    auto &instance(create_entity());
    instance.add_component<SpeciesComponent>(name, genus, epithet);
    species_reg.push_back(&instance.get_component<SpeciesComponent>());

    auto &species = instance.get_component<SpeciesComponent>();

    NC_LOG_INFO("Added species {} to the environment, with following traits:\n speed {}, energy capacity {}, size {}",
                species.get_name_formatted(true), species.genes.speed, species.genes.energy_capacity,
                species.genes.size);

    add_organism(&species);
}

SpeciesComponent *MicrocosmWorld::get_species_by_id(size_t entityId) {
    auto entity = get_entity_by_id(entityId);
    if (entity == nullptr) {
        NC_LOG_ERROR("Tried to get species with invalid entity ID {}!", entityId);
        return nullptr;
    }
    if (!entity->has_component<SpeciesComponent>()) {
        NC_LOG_ERROR("Entity {} has no species component!", entityId);
        return nullptr;
    }
    return &entity->get_component<SpeciesComponent>();
}

SpeciesComponent *MicrocosmWorld::get_species(const SpeciesComponent *species) {
    return get_species_by_id(species->entity->get_id());
}

void MicrocosmWorld::remove_species(SpeciesComponent *species) {
    if (species == nullptr) {
        NC_LOG_ERROR("Tried to make a null species extinct!");
        return;
    }

    const auto it = std::ranges::find(species_reg, species);
    if (it == species_reg.end()) {
        NC_LOG_ERROR("Species {} is not found!", species->entity->get_id());
        return;
    }

    const auto formattedName = species->get_name_formatted(false);

    species->entity->destroy();
    clear_organism_reg(species);
    species_reg.erase(it);

    NC_LOG_INFO("Species {} has gone extinct!", formattedName);
}

int MicrocosmWorld::get_species_idx(const SpeciesComponent *species) const {
    if (const auto it = std::ranges::find(species_reg, species); it != species_reg.end()) {
        return static_cast<int>(std::distance(species_reg.begin(), it));
    }

    NC_LOG_ERROR("Index of species {} is not found!", species != nullptr ? species->entity->get_id() : 0);
    return -1;
}

std::string MicrocosmWorld::get_species_name(OrganismComponent *organism) {
    auto species = get_species_by_id(organism->species_id);
    return species ? (species->genus + " " + species->epithet) : "Unknown";
}

OrganismComponent &MicrocosmWorld::add_organism(SpeciesComponent *species) {
    auto *viewport = get_main_loop().get_services().try_get<ncore::Viewport2D>();

    auto &instance(create_entity());

    float spawn_x = 0;
    float spawn_y = 0;

    // Get camera position from viewport
    if (viewport) {
        auto *camera = viewport->get_main_camera();
        if (camera) {
            const auto cameraPos = camera->get_position();
            spawn_x = cameraPos.x + ncore::Random::rand_float(-3.0f, 3.0f);
            spawn_y = cameraPos.y + ncore::Random::rand_float(-3.0f, 3.0f);
        }
    }

    auto &transform = instance.add_component<ncore::TransformComponent>(
        ncore::Vec2(spawn_x, spawn_y), 0.0f, ncore::Vec2(species->genes.size, species->genes.size));
    instance.add_component<ncore::RigidBodyComponent>();

    auto &organism = instance.add_component<OrganismComponent>(species);
    organism.species_id = species->entity->get_id();
    organism.cur_energy = organism.genome.energy_capacity;
    organism.fitness = 0;
    organism_reg.push_back(&organism);
    species->population_count++;

    instance.add_component<OrganismAIComponent>(organism.genome.speed, 1.5f);

    auto &circle = instance.add_component<ncore::TempCircleComponent>();
    circle.radius = organism.genome.size / 2.0f;
    circle.color = organism.genome.membrane_color;
    circle.filled = true;
    circle.edge = false;

    NC_LOG_INFO("Added organism of species {}, ID: {} with following "
                "traits:\n energy cap {}, speed {}, size {}, aggressiveness {}",
                species->get_name_formatted(false), organism.entity->get_id(), organism.genome.energy_capacity,
                organism.genome.speed, organism.genome.size, organism.genome.aggresiveness);

    // Play sound effect
    if (auto audio = get_system<ncore::AudioSystem>()) {
        audio->play_sound("assets/pop.wav");
    }

    return organism;
}

void MicrocosmWorld::remove_organism(OrganismComponent *organism) {
    if (organism == nullptr) {
        NC_LOG_ERROR("Tried to delete a null organism!");
        return;
    }

    const auto it = std::ranges::find(organism_reg, organism);
    if (it == organism_reg.end()) {
        NC_LOG_ERROR("Organism {} is not found!", organism->entity->get_id());
        return;
    }

    organism->entity->destroy();
    organism_reg.erase(it);
}

void MicrocosmWorld::clear_organism_reg(SpeciesComponent *species) {
    /* Proceed if not empty */
    if (!organism_reg.empty()) {
        for (auto &it: organism_reg)
            it->entity->destroy();

        organism_reg.clear();
    }
}
