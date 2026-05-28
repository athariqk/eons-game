#pragma once

#include <memory>
#include <string>

#include <Event.h>

class ImGuiLayer;
class MicrocosmScene;
struct SDL_Window;

class GUI {
public:
    GUI();
    ~GUI();

    void OnInit(SDL_Window *window);
    void OnRender(MicrocosmScene *scene, SDL_Window *window);
    void OnEvent(std::shared_ptr<BaseEvent> event) const;
    void OnClear();

    bool debugMode = false;

private:
    std::unique_ptr<ImGuiLayer> imgui;
    bool showCreateSpecies = false;

    std::string sGenus;
    std::string sEpithet;

    float xRel{};
    float yRel{};
};
