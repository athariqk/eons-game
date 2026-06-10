#include "MicrocosmWorld.h"

#include <algorithm>
#include <vector>

#include <SDL3/SDL_mouse.h>
#include <imgui.h>

#include <AudioSystem.h>
#include <InputEvents.h>
#include <InputSystem.h>
#include <Logger.h>
#include <RenderSystem.h>
#include <Services.h>
#include <Viewport.h>
#include <Window.h>

#include <Random.h>
#include <RigidBodyComponent.h>
#include <SpriteComponent.h>
#include <TransformComponent.h>
#include <components/OrganismAIComponent.h>
#include "components/NutrientComponent.h"
#include "components/OrganismComponent.h"
#include "components/SpeciesComponent.h"
#include "systems/NutrientSystem.h"
#include "systems/OrganismAISystem.h"
#include "systems/OrganismSystem.h"
#include "systems/SpeciesSystem.h"

MicrocosmWorld::MicrocosmWorld() : m_config(m_configFileName) { emptyInput(); }

void MicrocosmWorld::RegisterSystems() {
    // Register all game systems
    AddSystem<SpeciesSystem>();
    AddSystem<OrganismSystem>();
    AddSystem<NutrientSystem>();
    AddSystem<OrganismAISystem>();
}

void MicrocosmWorld::OnInit() {
    m_cameraConf = m_config.Load<Config::MicrocosmCamera>();

    RegisterSystems();

    m_inputSystem = GetSystem<Aeon::InputSystem>();
    m_viewport = GetMainLoop().GetServices().TryGet<Aeon::Viewport2D>();
    m_mainCamera = m_viewport ? m_viewport->GetMainCamera() : nullptr;
    m_renderSystem = GetSystem<Aeon::RenderSystem>();

    /* Initial species */
    AddSpeciesToEnvironment("Primum", "Primus", "specium");
}

void MicrocosmWorld::OnUpdate(const double p_delta) {
    if (m_inputSystem && m_viewport && m_mainCamera) {
        UpdateCameraControl(p_delta);
        UpdateCameraMovement(p_delta);
    }
}

void MicrocosmWorld::OnGuiRender() {
    ImGui::Begin("Microcosmos", nullptr, ImGuiWindowFlags_AlwaysAutoResize);

    if (ImGui::TreeNode("SpeciesList", "Species alive: %i", m_speciesEntities.size())) {
        for (int n = 0; n < m_speciesEntities.size(); n++) {
            ImGui::PushID(n);

            auto species = m_speciesEntities[n];

            if (ImGui::TreeNode("SpeciesNode", "%s", species->GetFormattedName(true).c_str())) {
                ImGui::BulletText("ID: %i", species->entity->GetID());
                ImGui::BulletText("Population: %i", species->populationCount);

                ImGui::TreePop();
            }

            if (ImGui::TreeNode("OrganismList", "Individuals")) {
                for (int i = 0; i < m_organismEntities.size(); i++) {
                    ImGui::PushID(i);
                    if (const auto organism = m_organismEntities[i];
                        ImGui::TreeNode("individuals", "Organism %i", organism->entity->GetID())) {

                        // Get AI component if it exists
                        std::string behaviour = "No AI";
                        if (organism->entity->HasComponent<OrganismAIComponent>()) {
                            auto &ai = organism->entity->GetComponent<OrganismAIComponent>();
                            behaviour = ai.getCurrentBehaviour();
                        }

                        auto &body = organism->entity->GetComponent<Aeon::RigidBodyComponent>();

                        ImGui::Text("Behaviour: %s\nEnergy: %.0f/%.0f\nSpeed: %f\nSize: %f\nAggressiveness: "
                                    "%f\nMembraneColour.r: %i\nMembraneColour.g: %i\nMembraneColour.b: "
                                    "%i\nFitness: %.0f\nMagnitude: %.0f",
                                    behaviour.c_str(), organism->curEnergy, organism->genome.energyCapacity,
                                    organism->genome.speed, organism->genome.size, organism->genome.aggresiveness,
                                    organism->genome.membraneColour.r, organism->genome.membraneColour.g,
                                    organism->genome.membraneColour.b, organism->fitness, body.velocity.Length());
                        ImGui::TreePop();
                    }
                    ImGui::PopID();
                }
                ImGui::TreePop();
            }

            if (ImGui::Button("Add Organism", ImVec2(100, 25))) {
                AddOrganism(species);
            }

            ImGui::SameLine();

            if (ImGui::Button("Make Extinct", ImVec2(100, 25))) {
                MakeExtinct(species);
            }

            ImGui::PopID();
        }
        ImGui::TreePop();
    }

    if (const auto &nutrients(GetGroup(MicrocosmWorld::NutrientsGroup));
        ImGui::TreeNode("NutrientList", "Nutrients available: %i", nutrients.size())) {
        for (int n = 0; n < nutrients.size(); n++) {
            ImGui::Text("Nutrient instance %.0f", nutrients[n]->GetComponent<NutrientComponent>().curEnergy);
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
                if (isInputEmpty(inputGenus) || isInputEmpty(inputEpithet)) {
                    LOG_ERROR(Aeon::Log::Game, "Genus or epithet is not valid!");
                } else {
                    AddSpeciesToEnvironment(inputGenus, inputGenus, inputEpithet);
                    emptyInput();
                    ImGui::CloseCurrentPopup();
                }
            }
            ImGui::SameLine();
            if (ImGui::Button("Random", ImVec2(80, 25))) {
            }
            ImGui::SameLine();
            if (ImGui::Button("Cancel", ImVec2(80, 25))) {
                ImGui::CloseCurrentPopup();
                emptyInput();
            }

            ImGui::EndPopup();
        }
    } /* ----- Create new species popup|end| ------- */

    ImGui::End();
}

void MicrocosmWorld::OnFinish() {}

void MicrocosmWorld::UpdateCameraControl(double p_delta) {
    const float delta = static_cast<float>(p_delta);

    m_camInputDir.Zero();

    if (m_inputSystem->IsKeyPressed(Aeon::KeyboardEvent::Key::W))
        m_camInputDir.y -= 1.0f;
    if (m_inputSystem->IsKeyPressed(Aeon::KeyboardEvent::Key::S))
        m_camInputDir.y += 1.0f;
    if (m_inputSystem->IsKeyPressed(Aeon::KeyboardEvent::Key::A))
        m_camInputDir.x -= 1.0f;
    if (m_inputSystem->IsKeyPressed(Aeon::KeyboardEvent::Key::D))
        m_camInputDir.x += 1.0f;

    const float lenSq = m_camInputDir.LengthSqr();
    if (lenSq > 0.001f)
        m_camInputDir *= 1.0f / std::sqrt(lenSq);

    m_isDragging = m_inputSystem->IsMouseButtonPressed(Aeon::ButtonIndex::Middle);

    if (m_isDragging) {
        const float zoom = m_mainCamera->GetZoom();
        const float ppm = m_viewport->GetPixelsPerMeter();
        const auto md = m_inputSystem->GetLastMouseDelta();

        auto targetVelocity = (-md / (ppm * zoom)) / delta;
        const float t = 1.0f - std::exp(-m_cameraConf.DragSensitivity * delta);
        m_camVelocity += (targetVelocity - m_camVelocity) * t;

        // Zero out WASD input so keyboard doesn't interfere
        m_camInputDir.Zero();
    }

    m_zoomInput = m_inputSystem->GetLastMouseWheelDelta().y;
}

void MicrocosmWorld::UpdateCameraMovement(const double p_delta) {
    const float delta = static_cast<float>(p_delta);

	// Movement
    if (!m_isDragging) {
        m_camVelocity += m_camInputDir * m_cameraConf.Acceleration * delta;

        const float friction = 1.0f - std::exp(-m_cameraConf.Friction * delta);
        m_camVelocity -= m_camVelocity * friction;

        const float speedSq = m_camVelocity.LengthSqr();
        if (speedSq > m_cameraConf.MaxSpeed * m_cameraConf.MaxSpeed)
            m_camVelocity *= m_cameraConf.MaxSpeed / std::sqrt(speedSq);
    }

    auto pos = m_mainCamera->GetPosition();
    pos += m_camVelocity * delta;
    m_mainCamera->SetPosition(pos);

    // Zoom
    m_zoomVelocity += m_zoomInput * m_cameraConf.ZoomSensitivity;
    const float zoomFriction = 1.0f - std::exp(-m_cameraConf.ZoomFriction * delta);
    m_zoomVelocity -= m_zoomVelocity * zoomFriction;

    const float newZoom =
        std::clamp(m_mainCamera->GetZoom() + m_zoomVelocity * delta, m_cameraConf.MinZoom, m_cameraConf.MaxZoom);

    m_mainCamera->SetZoom(newZoom);
}

void MicrocosmWorld::AddSpeciesToEnvironment(const std::string &name, const std::string &genus,
                                             const std::string &epithet) {
    auto &instance(CreateEntity());
    instance.AddComponent<SpeciesComponent>(name, genus, epithet);
    m_speciesEntities.push_back(&instance.GetComponent<SpeciesComponent>());

    auto &species = instance.GetComponent<SpeciesComponent>();

    LOG_INFO(Aeon::Log::Game, "Added species {} to the environment, with following traits:\n speed {}, energy capacity {}, size {}",
             species.GetFormattedName(true), species.genes.speed, species.genes.energyCapacity, species.genes.size);

    AddOrganism(&species);
}

SpeciesComponent *MicrocosmWorld::GetSpeciesById(size_t entityId) {
    auto entity = GetEntityById(entityId);
    if (entity == nullptr) {
        LOG_ERROR(Aeon::Log::Game, "Tried to get species with invalid entity ID {}!", entityId);
        return nullptr;
    }
    if (!entity->HasComponent<SpeciesComponent>()) {
        LOG_ERROR(Aeon::Log::Game, "Entity {} has no species component!", entityId);
        return nullptr;
    }
    return &entity->GetComponent<SpeciesComponent>();
}

SpeciesComponent *MicrocosmWorld::GetSpecies(const SpeciesComponent *species) {
    return GetSpeciesById(species->entity->GetID());
}

void MicrocosmWorld::MakeExtinct(SpeciesComponent *species) {
    if (species == nullptr) {
        LOG_ERROR(Aeon::Log::Game, "Tried to make a null species extinct!");
        return;
    }

    const auto it = std::ranges::find(m_speciesEntities, species);
    if (it == m_speciesEntities.end()) {
        LOG_ERROR(Aeon::Log::Game, "Species {} is not found!", species->entity->GetID());
        return;
    }

    const auto formattedName = species->GetFormattedName(false);

    species->entity->Destroy();
    ClearOrganisms(species);
    m_speciesEntities.erase(it);

    LOG_INFO(Aeon::Log::Game, "Species {} has gone extinct!", formattedName);
}

int MicrocosmWorld::GetSpeciesIndex(const SpeciesComponent *species) const {
    if (const auto it = std::ranges::find(m_speciesEntities, species); it != m_speciesEntities.end()) {
        return static_cast<int>(std::distance(m_speciesEntities.begin(), it));
    }

    LOG_ERROR(Aeon::Log::Game, "Index of species {} is not found!", species != nullptr ? species->entity->GetID() : 0);
    return -1;
}

std::string MicrocosmWorld::GetSpeciesName(OrganismComponent *organism) {
    auto species = GetSpeciesById(organism->speciesId);
    return species ? (species->genus + " " + species->epithet) : "Unknown";
}

OrganismComponent &MicrocosmWorld::AddOrganism(SpeciesComponent *species) {
    auto *viewport = GetMainLoop().GetServices().TryGet<Aeon::Viewport2D>();

    auto &instance(CreateEntity());

    float spawnX = 0;
    float spawnY = 0;

    // Get camera position from viewport
    if (viewport) {
        auto *camera = viewport->GetMainCamera();
        if (camera) {
            const auto cameraPos = camera->GetPosition();
            spawnX = cameraPos.x + Aeon::Random::RandomFloat(-3.0f, 3.0f);
            spawnY = cameraPos.y + Aeon::Random::RandomFloat(-3.0f, 3.0f);
        }
    }

    auto &transform = instance.AddComponent<Aeon::TransformComponent>(
        Aeon::Vector2D(spawnX, spawnY), 0.0f, Aeon::Vector2D(species->genes.size, species->genes.size));
    instance.AddComponent<Aeon::RigidBodyComponent>();

    auto &organism = instance.AddComponent<OrganismComponent>(species);
    organism.speciesId = species->entity->GetID();
    organism.curEnergy = organism.genome.energyCapacity;
    organism.fitness = 0;
    m_organismEntities.push_back(&organism);
    species->populationCount++;

    instance.AddComponent<OrganismAIComponent>(organism.genome.speed, 1.5f);

    auto &tempCirc = instance.AddComponent<Aeon::TempCircleComponent>();
    tempCirc.radius = organism.genome.size / 2.0f;
    tempCirc.color = organism.genome.membraneColour;
    tempCirc.filled = true;
    tempCirc.edge = false;

    LOG_INFO(Aeon::Log::Game, "Added organism of species {}, ID: {} with following "
             "traits:\n energy cap {}, speed {}, size {}, aggressiveness {}",
             species->GetFormattedName(false), organism.entity->GetID(), organism.genome.energyCapacity,
             organism.genome.speed, organism.genome.size, organism.genome.aggresiveness);

    // Play sound effect
    if (auto audioSystem = GetSystem<Aeon::AudioSystem>()) {
        audioSystem->PlaySound("assets/pop.wav");
    }

    return organism;
}

void MicrocosmWorld::DeleteOrganism(OrganismComponent *organism) {
    if (organism == nullptr) {
        LOG_ERROR(Aeon::Log::Game, "Tried to delete a null organism!");
        return;
    }

    const auto it = std::ranges::find(m_organismEntities, organism);
    if (it == m_organismEntities.end()) {
        LOG_ERROR(Aeon::Log::Game, "Organism {} is not found!", organism->entity->GetID());
        return;
    }

    organism->entity->Destroy();
    m_organismEntities.erase(it);
}

void MicrocosmWorld::ClearOrganisms(SpeciesComponent *species) {
    /* Proceed if not empty */
    if (!m_organismEntities.empty()) {
        for (auto &it: m_organismEntities)
            it->entity->Destroy();

        m_organismEntities.clear();
    }
}

