#pragma once

#include <array>
#include <memory>

#include <ncore/modules/video/viewport.h>
#include "main_loop.h"

namespace ncore {

class AssetManager;
class IWindowService;
class IRenderService;
class IPhysicsService;
class IPhysicsService;
class IAudioService;
class IGuiService;
struct Vec2;

namespace cfg {

struct Window {
    int SizeWidth = 800;
    int SizeHeight = 800;
    bool Fullscreen = false;
    NC_BIND(Window, NC_F(Window, SizeWidth) NC_F(Window, SizeHeight) NC_F(Window, Fullscreen));
};

struct Render {
    float PixelsPerMeter = 32.0f;
    NC_BIND(Render, NC_F(Render, PixelsPerMeter));
};

struct Runtime {
    std::string DefaultWorld;
    NC_BIND(Runtime, NC_F(Runtime, DefaultWorld));
};

} // namespace cfg

/**
 * @brief The default main loop implementation for game apps.
 * This owns hardware resources and services
 */
class GameLoop : public MainLoop {
    NCLASS(GameLoop, MainLoop)

public:
    GameLoop(std::string app_name) : MainLoop(std::move(app_name)) {}

    void log(const std::string &message);
    void log_error(const std::string &message);

protected:
    Error init(IWorld &world) override;
    Error main_loop() override;
    Error poll_events() override;
    Error cleanup() override;

private:
    AssetManager *resources = nullptr;
    IRenderService *renderer = nullptr;
    IWindowService *window = nullptr;
    IPhysicsService *physics = nullptr;
    IAudioService *audio = nullptr;
    IGuiService *gui = nullptr;

    // todo: fix this, shouldnt have a viwport here...
    std::unique_ptr<Viewport> viewport;

    uint64_t ticks = 0;
    int cur_ticks = 0;
    std::array<char, 100> attrs{};
};

} // namespace ncore
