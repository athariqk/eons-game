#pragma once

struct SDL_Window;
union SDL_Event;

class ImGuiLayer {
public:
    explicit ImGuiLayer(SDL_Window *window);
    ~ImGuiLayer() {}

    void OnEvent(const SDL_Event &event);
    void Begin();
    void End(SDL_Window *window);
    void Clear();
};
