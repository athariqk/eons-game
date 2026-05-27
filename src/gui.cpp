#include "gui.h"

#include "scene.h"

#include "Simulation/environment.h"
#include "Simulation/organism.h"
#include "Simulation/organismAI.h"
#include "Simulation/species.h"

#include "Logger.h"

#include "ImGui/ImGuiLayer.h"

#include <imgui.h>
#include <misc/cpp/imgui_stdlib.h>

#include <SDL3/SDL_mouse.h>

#include "Camera.h"

GUI::GUI() = default;

GUI::~GUI() = default;

void GUI::OnInit() { imgui = std::make_unique<ImGuiLayer>(Scene::Get()->GetWindow().GetWindow()); }

void GUI::OnImGuiEvent() const {
    if (imgui != nullptr) {
        imgui->OnEvent(Scene::Get()->m_event);
    }
}

void GUI::OnImGuiRender() {
    if (imgui == nullptr) {
        return;
    }

    imgui->Begin();
    {
        /* ----- Environment Window|begin| ------- */
        ImGui::Begin("Environment", nullptr, ImGuiWindowFlags_AlwaysAutoResize);

        auto species = Scene::Get()->GetEnvironment().getAllSpecies();

        if (ImGui::TreeNode("SpeciesList", "Species present: %i", species.size())) {
            for (int s = 0; s < species.size(); s++) {
                ImGui::PushID(s);

                if (ImGui::TreeNode("specificSpecies", "%s, Pop: %i", species[s]->getFormattedName(false).c_str(),
                                    species[s]->getPopulationCount())) {
                    if (ImGui::TreeNode("organismsList", "Organisms")) {
                        for (int i = 0; i < species[s]->organisms.size(); i++) {
                            ImGui::PushID(i);
                            if (const auto organism = species[s]->organisms[i];
                                ImGui::TreeNode("individuals", "Organism %i", organism->getID())) {
                                ImGui::Text("Behaviour: %s\nEnergy: %.0f/%.0f\nSpeed: %f\nSize: %f\nAggressiveness: "
                                            "%f\nMembraneColour.r: %i\nMembraneColour.g: %i\nMembraneColour.rb: "
                                            "%i\nFitness: %.0f",
                                            organism->ai->getCurrentBehaviour().c_str(), organism->curEnergy,
                                            organism->genome.energyCapacity, organism->genome.speed,
                                            organism->genome.size, organism->genome.aggresiveness,
                                            organism->genome.membraneColour.r, organism->genome.membraneColour.g,
                                            organism->genome.membraneColour.b, organism->fitness);
                                ImGui::TreePop();
                            }
                            ImGui::PopID();
                        }
                        ImGui::TreePop();
                    }

                    if (ImGui::Button("Add Organism", ImVec2(100, 25))) {
                        species[s]->addOrganism();
                    }

                    ImGui::SameLine();

                    if (ImGui::Button("Make Extinct", ImVec2(100, 25))) {
                        Scene::Get()->GetEnvironment().makeExtinct(species[s]);
                    }

                    ImGui::TreePop();
                }
                ImGui::PopID();
            }
            ImGui::TreePop();
        }

        if (const auto &nutrients(Scene::Get()->GetEntityManager().GetGroup(Scene::groupLabels::NutrientsGroup));
            ImGui::TreeNode("NutrientList", "Nutrients available: %i", nutrients.size())) {
            for (int n = 0; n < nutrients.size(); n++) {
                ImGui::Text("Nutrient instance %.0f", nutrients[n]->GetComponent<Nutrient>().curEnergy);
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
                ImGui::InputText("Genus", &sGenus);
                ImGui::InputText("Epithet", &sEpithet);
                ImGui::Separator();
                if (ImGui::Button("Create", ImVec2(80, 25))) {
                    if (sGenus.empty() || sEpithet.empty()) {
                        LOG_ERROR("Genus or epithet is not valid!");
                    } else {
                        Scene::Get()->GetEnvironment().addSpeciesToEnvironment(sGenus, sGenus, sEpithet);
                        sGenus.clear();
                        sEpithet.clear();
                        ImGui::CloseCurrentPopup();
                    }
                }
                ImGui::SameLine();
                if (ImGui::Button("Random", ImVec2(80, 25))) {
                }
                ImGui::SameLine();
                if (ImGui::Button("Cancel", ImVec2(80, 25))) {
                    ImGui::CloseCurrentPopup();
                    sGenus.clear();
                    sEpithet.clear();
                }

                ImGui::EndPopup();
            }
        }
        /* ----- Create new species popup|end| ------- */

        ImGui::SameLine();

        if (ImGui::Button("Add nutrients", ImVec2(120, 25))) {
            Scene::Get()->GetEnvironment().spawnNutrients(10);
        }

        ImGui::End();
        /* ----- Environment Window|end| ------- */
    }
    {
        /* ----- Simulation Window|begin| ------ */
        ImGui::Begin("Simulation", nullptr, ImGuiWindowFlags_AlwaysAutoResize);

        ImGui::Button("Reset", ImVec2(100, 20));

        ImGui::End();
        /* ----- Simulation Window|end| -------- */
    }
    {
        /* ----- Debug Window|begin| --------- */
        ImGui::Begin("Debug");

        auto &entities = Scene::Get()->GetEntityManager().GetEntities();

        ImGui::Checkbox("Debug mode", &debugMode);

        if (ImGui::TreeNode("entitiesList", "All entities present: %i", entities.size())) {
            for (int i = 0; i < entities.size(); i++) {
                ImGui::PushID(i);

                ImGui::Text("Entity instance");

                ImGui::PopID();
            }
            ImGui::TreePop();
        }

        ImGui::Text("Tick: %i", Scene::Get()->GetTick());

        const auto res = Scene::Get()->GetWindow().GetResolution();
        ImGui::Text("Window resolution: (x: %f, y: %f)", res.x, res.y);

        const auto camPos = Scene::Get()->GetCamera().position;
        ImGui::Text("Camera position: (x: %f, y: %f)", camPos.x, camPos.y);

        ImGui::End();
        /* ----- Debug Window|end| --------- */
    }

    imgui->End(Scene::Get()->GetWindow().GetWindow());
}

void GUI::OnImGuiClear() {
    if (imgui != nullptr) {
        imgui->Clear();
        imgui.reset();
    }
}
