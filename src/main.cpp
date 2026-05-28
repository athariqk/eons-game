#include "config.h"

#include <SDL3/SDL_render.h>
#include <SDL3/SDL_timer.h>

#include <array>
#include <cstdio>
#include <memory>

#include <Logger.h>
#include <MainLoop.h>
#include <Window.h>

#include "Microcosm.h"

int main(int argc, char *argv[]) {
    Logger::Init();
    LOG_TRACE("Log initialized");

    constexpr char title[20] = "Eons";
    Window window(title, WINDOW_WIDTH, WINDOW_HEIGHT, false);

    // Report info
    LOG_INFO("SDL Renderer name: {}", SDL_GetRendererName(window.GetRenderer()));
    LOG_TRACE("----- End of system information -----");

    MainLoop mainLoop(window);
    mainLoop.Init();

    // Set the initial scene
    auto microcosmScene = std::make_unique<MicrocosmScene>();
    mainLoop.SetCurrentScene(std::move(microcosmScene));

    constexpr int fps = 60;
    constexpr int frameDelay = 1000 / fps;
    double delta = 0;

    uint32_t lastFPSUpdateTime = 0;
    int frameCount = 0;
    float currentFPS = 0.0f;

    while (mainLoop.running()) {
        const uint32_t frameStart = SDL_GetTicks();

        mainLoop.HandleEvents();
        mainLoop.Update(delta, mainLoop.GetTick());
        mainLoop.Render();

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
    }

    mainLoop.Clean();

    return 0;
}
