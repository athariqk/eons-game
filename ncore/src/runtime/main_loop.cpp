#include <ncore/runtime/main_loop.h>

#include <memory>
#include <sstream>
#include <string>

#include <kernel/errors.h>
#include <kernel/world.h>
#include <runtime/register_types.h>
#include <runtime/service_locator.h>
#include <utils/assert.h>
#include <utils/logger/log_level.h>
#include <utils/logger/logger.h>
#include <utils/logger/sink.h>
#include <utils/macro.h>

namespace ncore {

MainLoop::MainLoop(std::string app_name) :
    app_name(std::move(app_name)), cfg_file(this->app_name), services(ServiceLocator::get_instance()) {
    event_bus.subscribe<MainLoopStopEvent>([this](MainLoopStopEvent &) {
        is_running = false;
        NC_LOG_TRACE("stop event received, stopping main loop...");
    });

    register_kernel_types();
}

MainLoop::~MainLoop() { unregister_kernel_types(); }

Error MainLoop::run(IWorld *world) {
    NC_ASSERT_RETVAL(world != nullptr, Error::FATAL, "MainLoop::run called with null world!");

    // Setup logging
    auto cfg_log = cfg_file.read<cfg::Log>();
    log::Logger::get_instance().add_sink(std::make_shared<log::FileSink>(cfg_log.FilePath));
    log::Logger::get_instance().set_level(log::Level(cfg_log.Level));
    if (!cfg_log.Overrides.empty()) {
        std::istringstream stream(cfg_log.Overrides);
        std::string pair;
        while (std::getline(stream, pair, ',')) {
            auto sep = pair.find(':');
            if (sep != std::string::npos) {
                auto cat = pair.substr(0, sep);
                auto lvl = std::stoi(pair.substr(sep + 1));
                log::Logger::get_instance().set_level(cat, log::Level(lvl));
            }
        }
    }

    active_world = world;

    auto result = init(*active_world);
    if (result != Error::OK)
        return result;
    is_initialized = true;
    is_running = true;

    result = main_loop();
    cleanup();
    return result;
}

IWorld &MainLoop::get_world() {
    NC_ASSERT(active_world != nullptr, "no active world set!");
    return *active_world;
}

} // namespace ncore
