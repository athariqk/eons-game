#pragma once

#include <memory>
#include <string>

class ImGuiLayer;

class GUI {
public:
    GUI();
    ~GUI();

    void OnInit();
    void OnImGuiRender();
    void OnImGuiEvent() const;
    void OnImGuiClear();

    bool debugMode = false;

private:
    std::unique_ptr<ImGuiLayer> imgui;
    bool showCreateSpecies = false;

    std::string sGenus;
    std::string sEpithet;

    float xRel{};
    float yRel{};
};
