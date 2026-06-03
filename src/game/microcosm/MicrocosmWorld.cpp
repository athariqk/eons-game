#include "MicrocosmWorld.h"

#include <algorithm>
#include <vector>

#include <SDL3/SDL_mouse.h>
#include <imgui.h>

#include <AudioSystem.h>
#include <InputEvents.h>
#include <InputSystem.h>
#include <Logger.h>
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

MicrocosmWorld::MicrocosmWorld() { emptyInput(); }

void MicrocosmWorld::RegisterSystems() {
    // Register all game systems
    AddSystem<SpeciesSystem>();
    AddSystem<OrganismSystem>();
    AddSystem<NutrientSystem>();
    AddSystem<OrganismAISystem>();
}

void MicrocosmWorld::OnInit() {
    RegisterSystems();

    // Subscribe to events via EventBus
    SubscribeToEvents();

    SpawnNutrients(30);

    /* Initial species */
    AddSpeciesToEnvironment("Primum", "Primus", "specium");
}

void MicrocosmWorld::SubscribeToEvents() {
    auto &eventBus = GetMainLoop().GetEventBus();

    auto viewport = GetMainLoop().GetServices().TryGet<Aeon::Viewport2D>();
    if (!viewport)
        return;

    auto camera = viewport->GetMainCamera();

    auto ppm = viewport->GetPixelsPerMeter();

    // Subscribe to mouse motion for camera panning
    m_mouseMotionSub =
        eventBus.Subscribe<Aeon::MouseMotionEvent>([this, camera, ppm](const Aeon::MouseMotionEvent &event) {
            if (!camera)
                return;

            if (event.buttonState & SDL_BUTTON_LMASK) {
                float zoom = camera->GetZoom();

                // Direct world-space delta from this frame's drag
                float worldDeltaX = -(event.delta.x) / (ppm * zoom);
                float worldDeltaY = -(event.delta.y) / (ppm * zoom);

                auto pos = camera->GetPosition();
                pos.x += worldDeltaX;
                pos.y += worldDeltaY;
                camera->SetPosition(pos);

                // Feed swipe velocity for post-release momentum
                // Blend toward current frame delta so fast swipes win
                m_swipeVelocity.x = m_swipeVelocity.x * 0.4f + (worldDeltaX / 0.016f) * 0.6f * swipeSensitivity;
                m_swipeVelocity.y = m_swipeVelocity.y * 0.4f + (worldDeltaY / 0.016f) * 0.6f * swipeSensitivity;

                m_isDragging = true;
            } else {
                m_isDragging = false;
            }
        });

    // Subscribe to mouse wheel for zooming
    m_mouseWheelSub = eventBus.Subscribe<Aeon::MouseWheelEvent>([this, camera](const Aeon::MouseWheelEvent &event) {
        if (!camera)
            return;

        float currentZoom = camera->GetZoom();

        constexpr float zoomSensitivity = 0.1f;
        currentZoom += event.scrollY * zoomSensitivity;

        constexpr float minZoom = 0.1f;
        constexpr float maxZoom = 4.0f;
        currentZoom = std::clamp(currentZoom, minZoom, maxZoom);

        camera->SetZoom(currentZoom);
    });
}

void MicrocosmWorld::OnUpdate(const double p_delta) { UpdateCameraMovement(p_delta); }

void MicrocosmWorld::OnGuiRender() {
    /* ----- Environment Window|begin| ------- */
    {
        ImGui::Begin("Environment", nullptr, ImGuiWindowFlags_AlwaysAutoResize);

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
                        LOG_ERROR("Genus or epithet is not valid!");
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

        ImGui::SameLine();

        if (ImGui::Button("Add nutrients", ImVec2(120, 25))) {
            SpawnNutrients(10);
        }

        ImGui::End();

    } /* ----- Environment Window|end| ------- */

    /* ----- Simulation Window|begin| ------ */
    {

        ImGui::Begin("Simulation", nullptr, ImGuiWindowFlags_AlwaysAutoResize);

        ImGui::Button("Reset", ImVec2(100, 20));

        ImGui::End();

    } /* ----- Simulation Window|end| -------- */

    /* ----- Debug Window|begin| --------- */
    {
        ImGui::Begin("Debug");

        // Note: Direct entity access would need to be exposed through scene
        // For now, showing limited debug info

        ImGui::Checkbox("Debug mode", &debugMode);

        // Get camera from viewport service
        auto *viewport = GetMainLoop().GetServices().TryGet<Aeon::Viewport2D>();
        if (viewport) {
            auto *cam = viewport->GetMainCamera();
            if (cam) {
                const auto &camPos = cam->GetPosition();
                ImGui::Text("Camera position: (x: %f, y: %f)", camPos.x, camPos.y);

                float zoom = cam->GetZoom();
                ImGui::Text("Camera zoom: %f", zoom);
            }
        }

        ImGui::End();
    } /* ----- Debug Window|end| --------- */
}

void MicrocosmWorld::OnFinish() {
    // Unsubscribe from events
    auto &eventBus = GetMainLoop().GetEventBus();
    eventBus.Unsubscribe(m_mouseMotionSub);
    eventBus.Unsubscribe(m_keyboardSub);
    eventBus.Unsubscribe(m_mouseButtonSub);
    eventBus.Unsubscribe(m_mouseWheelSub);
}

void MicrocosmWorld::UpdateCameraMovement(const double p_delta) {
    auto *viewport = GetMainLoop().GetServices().TryGet<Aeon::Viewport2D>();
    if (!viewport)
        return;

    auto *camera = viewport->GetMainCamera();
    if (!camera)
        return;

    auto *inputSystem = GetSystem<Aeon::InputSystem>();
    if (!inputSystem)
        return;

    float delta = static_cast<float>(p_delta);

    Aeon::Vector2D inputDir(0.0f, 0.0f);

    if (inputSystem->IsKeyPressed(Aeon::KeyboardEvent::Key::W))
        inputDir.y -= 1.0f;
    if (inputSystem->IsKeyPressed(Aeon::KeyboardEvent::Key::S))
        inputDir.y += 1.0f;
    if (inputSystem->IsKeyPressed(Aeon::KeyboardEvent::Key::A))
        inputDir.x -= 1.0f;
    if (inputSystem->IsKeyPressed(Aeon::KeyboardEvent::Key::D))
        inputDir.x += 1.0f;

    if (inputDir.LengthSqr() > 0.001f) {
        float len = inputDir.Length();
        inputDir.x /= len;
        inputDir.y /= len;
    }

    m_cameraVelocity.x += inputDir.x * acceleration * delta;
    m_cameraVelocity.y += inputDir.y * acceleration * delta;

    float frictionFactor = 1.0f - std::exp(-friction * delta);
    m_cameraVelocity.x -= m_cameraVelocity.x * frictionFactor;
    m_cameraVelocity.y -= m_cameraVelocity.y * frictionFactor;

    // Speed cap
    float speedSq = m_cameraVelocity.LengthSqr();
    if (speedSq > maxSpeed * maxSpeed) {
        float inv = maxSpeed / std::sqrt(speedSq);
        m_cameraVelocity.x *= inv;
        m_cameraVelocity.y *= inv;
    }

    if (!m_isDragging) {
        float swipeFrictionFactor = 1.0f - std::exp(-swipeFriction * delta);
        m_swipeVelocity.x -= m_swipeVelocity.x * swipeFrictionFactor;
        m_swipeVelocity.y -= m_swipeVelocity.y * swipeFrictionFactor;

        if (m_swipeVelocity.LengthSqr() < 0.0001f) {
            m_swipeVelocity.x = 0.0f;
            m_swipeVelocity.y = 0.0f;
        }
    } else {
        m_cameraVelocity.x *= 0.85f;
        m_cameraVelocity.y *= 0.85f;
    }

    auto pos = camera->GetPosition();
    pos.x += (m_cameraVelocity.x + m_swipeVelocity.x) * delta;
    pos.y += (m_cameraVelocity.y + m_swipeVelocity.y) * delta;
    camera->SetPosition(pos);
}

void MicrocosmWorld::AddSpeciesToEnvironment(const std::string &name, const std::string &genus,
                                             const std::string &epithet) {
    auto &instance(CreateEntity());
    instance.AddComponent<SpeciesComponent>(name, genus, epithet);
    m_speciesEntities.push_back(&instance.GetComponent<SpeciesComponent>());

    auto &species = instance.GetComponent<SpeciesComponent>();

    LOG_INFO("Added species {} to the environment, with following traits:\n speed {}, energy capacity {}, size {}",
             species.GetFormattedName(true), species.genes.speed, species.genes.energyCapacity, species.genes.size);

    AddOrganism(&species);
}

SpeciesComponent *MicrocosmWorld::GetSpeciesById(size_t entityId) {
    auto entity = GetEntityById(entityId);
    if (entity == nullptr) {
        LOG_ERROR("Tried to get species with invalid entity ID {}!", entityId);
        return nullptr;
    }
    if (!entity->HasComponent<SpeciesComponent>()) {
        LOG_ERROR("Entity {} has no species component!", entityId);
        return nullptr;
    }
    return &entity->GetComponent<SpeciesComponent>();
}

SpeciesComponent *MicrocosmWorld::GetSpecies(const SpeciesComponent *species) {
    return GetSpeciesById(species->entity->GetID());
}

void MicrocosmWorld::MakeExtinct(SpeciesComponent *species) {
    if (species == nullptr) {
        LOG_ERROR("Tried to make a null species extinct!");
        return;
    }

    const auto it = std::ranges::find(m_speciesEntities, species);
    if (it == m_speciesEntities.end()) {
        LOG_ERROR("Species {} is not found!", species->entity->GetID());
        return;
    }

    const auto formattedName = species->GetFormattedName(false);

    species->entity->Destroy();
    ClearOrganisms(species);
    m_speciesEntities.erase(it);

    LOG_INFO("Species {} has gone extinct!", formattedName);
}

int MicrocosmWorld::GetSpeciesIndex(const SpeciesComponent *species) const {
    if (const auto it = std::ranges::find(m_speciesEntities, species); it != m_speciesEntities.end()) {
        return static_cast<int>(std::distance(m_speciesEntities.begin(), it));
    }

    LOG_ERROR("Index of species {} is not found!", species != nullptr ? species->entity->GetID() : 0);
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

    LOG_INFO("Added organism of species {}, ID: {} with following "
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
        LOG_ERROR("Tried to delete a null organism!");
        return;
    }

    const auto it = std::ranges::find(m_organismEntities, organism);
    if (it == m_organismEntities.end()) {
        LOG_ERROR("Organism {} is not found!", organism->entity->GetID());
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

void MicrocosmWorld::SpawnNutrients(int amount) {
    auto *viewport = GetMainLoop().GetServices().TryGet<Aeon::Viewport2D>();

    for (int i = 0; i < amount; i++) {
        auto &nutrient(CreateEntity());

        float size = Aeon::Random::RandomFloat(0.2f, 0.6f);

        float spawnX = 0;
        float spawnY = 0;

        if (viewport) {
            auto *camera = viewport->GetMainCamera();
            if (camera) {
                const auto cameraPos = camera->GetPosition();
                spawnX = cameraPos.x + Aeon::Random::RandomFloat(-12.5f, 12.5f);
                spawnY = cameraPos.y + Aeon::Random::RandomFloat(-12.5f, 12.5f);
            }
        }

        nutrient.AddComponent<Aeon::TransformComponent>(Aeon::Vector2D(spawnX, spawnY), 0.0f,
                                                        Aeon::Vector2D(size, size));
        nutrient.AddComponent<Aeon::RigidBodyComponent>();
        nutrient.AddComponent<NutrientComponent>(5.0f);
        nutrient.AddComponent<Aeon::SpriteComponent>("assets/nutrient.webp");
        nutrient.AddGroup(GroupLabels::NutrientsGroup);
    }

    LOG_INFO("Spawned {} nutrients to the environment", amount);
}
