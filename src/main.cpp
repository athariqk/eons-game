#include "config.h"
#include "scene.h"

#include <SDL3/SDL_render.h>
#include <SDL3/SDL_timer.h>

#include <array>
#include <cstdio>

#include "Logger.h"

int main(int argc, char *argv[]) {
    Logger::Init();
    LOG_TRACE("Log initialized");

    constexpr char title[20] = "kGeneticAlgorithm";
    Window window(title, WINDOW_WIDTH, WINDOW_HEIGHT, false);

    // Report info
    LOG_INFO("SDL Renderer name: {}", SDL_GetRendererName(window.GetRenderer()));
    LOG_TRACE("----- End of system information -----");

    Scene scene(window);
    scene.Init();

    constexpr int fps = 60;
    constexpr int frameDelay = 1000 / fps;
    double delta = 0;

    Uint32 lastFPSUpdateTime = 0;
    int frameCount = 0;
    float currentFPS = 0.0f;

    uint64_t ticks = 0;

    while (Scene::Get()->running()) {
        const uint32_t frameStart = SDL_GetTicks();

        scene.HandleEvents();
        scene.Update(delta, ticks);
        scene.Render();

        uint32_t currentTime = SDL_GetTicks();
        uint32_t frameTime = currentTime - frameStart;

        if (frameTime < frameDelay) {
            SDL_Delay(frameDelay - frameTime);
            currentTime = SDL_GetTicks();
            frameTime = currentTime - frameStart;
        }

        delta = static_cast<double>(frameTime) / 1000.0;

        if (currentTime - lastFPSUpdateTime > 1000) {
            // Update FPS once per second
            currentFPS = frameCount * 1000.0f / (currentTime - lastFPSUpdateTime);
            frameCount = 0;
            lastFPSUpdateTime = currentTime;

            std::array<char, 100> titleWAttrs{};
            std::snprintf(titleWAttrs.data(), titleWAttrs.size(), "%s - FPS: %.1f - Delta: %.5f", title, currentFPS,
                          delta);
            window.SetTitle(titleWAttrs.data());
        }

        frameCount++;
        ticks++;
    }

    Scene::Clean();

    return 0;
}
