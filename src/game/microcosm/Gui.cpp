#include "Gui.h"

#include <SDL3/SDL_mouse.h>

#include <imgui.h>
#include <misc/cpp/imgui_stdlib.h>

#include <ImGuiLayer.h>
#include <Logger.h>
#include <Viewport.h>

#include "Microcosm.h"
#include "Nutrient.h"
#include "Organism.h"
#include "OrganismAI.h"
#include "Species.h"

GUI::GUI() = default;

GUI::~GUI() = default;

void GUI::OnInit(SDL_Window *window) { imgui = std::make_unique<ImGuiLayer>(window); }

void GUI::OnEvent(std::shared_ptr<BaseEvent> event) const { imgui->OnEvent(event->m_handle); }

void GUI::OnRender(MicrocosmScene *scene, SDL_Window *window) {
    if (imgui == nullptr || scene == nullptr) {
        return;
    }

    imgui->Begin();
    {
        /* ----- Environment Window|begin| ------- */
        ImGui::Begin("Environment", nullptr, ImGuiWindowFlags_AlwaysAutoResize);

        auto &allSpecies = scene->GetAllSpecies();

        if (ImGui::TreeNode("SpeciesList", "Species alive: %i", allSpecies.size())) {
            for (int n = 0; n < allSpecies.size(); n++) {
                ImGui::PushID(n);

                auto &species = *allSpecies[n];

                if (ImGui::TreeNode("SpeciesNode", "%s", species.getFormattedName(true).c_str())) {
                    ImGui::BulletText("ID: %i", species.getID());
                    ImGui::BulletText("Population: %i", species.getPopulationCount());

                    ImGui::TreePop();
                }

                if (ImGui::TreeNode("OrganismList", "Individuals")) {
                    for (int i = 0; i < species.organisms.size(); i++) {
                        ImGui::PushID(i);
                        if (const auto organism = species.organisms[i];
                            ImGui::TreeNode("individuals", "Organism %i", organism->getID())) {
                            ImGui::Text("Behaviour: %s\nEnergy: %.0f/%.0f\nSpeed: %f\nSize: %f\nAggressiveness: "
                                        "%f\nMembraneColour.r: %i\nMembraneColour.g: %i\nMembraneColour.rb: "
                                        "%i\nFitness: %.0f",
                                        organism->ai->getCurrentBehaviour().c_str(), organism->curEnergy,
                                        organism->genome.energyCapacity, organism->genome.speed, organism->genome.size,
                                        organism->genome.aggresiveness, organism->genome.membraneColour.r,
                                        organism->genome.membraneColour.g, organism->genome.membraneColour.b,
                                        organism->fitness);
                            ImGui::TreePop();
                        }
                        ImGui::PopID();
                    }
                    ImGui::TreePop();
                }

                if (ImGui::Button("Add Organism", ImVec2(100, 25))) {
                    species.addOrganism();
                }

                ImGui::SameLine();

                if (ImGui::Button("Make Extinct", ImVec2(100, 25))) {
                    scene->MakeExtinct(&species);
                }

				ImGui::PopID();
            }
            ImGui::TreePop();
        }

        if (const auto &nutrients(scene->GetGroup(MicrocosmScene::NutrientsGroup));
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
                        scene->AddSpeciesToEnvironment(sGenus, sGenus, sEpithet);
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
            scene->SpawnNutrients(10);
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

        // Note: Direct entity access would need to be exposed through scene
        // For now, showing limited debug info

        ImGui::Checkbox("Debug mode", &debugMode);

        auto cam = scene->GetViewport()->GetMainCamera();

        // Camera position from MicrocosmScene
        const auto &camPos = cam->GetPosition();
        ImGui::Text("Camera position: (x: %f, y: %f)", camPos.x, camPos.y);

        float zoom = cam->GetZoom();
        ImGui::Text("Camera zoom: %f", zoom);

        ImGui::End();
        /* ----- Debug Window|end| --------- */
    }

    imgui->End(window);
}

void GUI::OnClear() {
    if (imgui != nullptr) {
        imgui->Clear();
        imgui.reset();
    }
}
