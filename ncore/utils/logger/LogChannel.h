#pragma once

#include <format>
#include <memory>
#include <vector>

#include "LogLevel.h"
#include "Sink.h"

namespace ncore::log {

// Engine modules
inline constexpr std::string_view DEFAULT = "NCORE";
inline constexpr std::string_view AUDIO = "AUDIO";
inline constexpr std::string_view ECS = "ECS";
inline constexpr std::string_view EVENTS = "EVENTS";
inline constexpr std::string_view GRAPHICS = "GRAPHS";
inline constexpr std::string_view GUI = "GUI";
inline constexpr std::string_view PHYSICS = "PHYS";
inline constexpr std::string_view IO = "IO";

class Sink;

class LogChannel {
public:
    LogChannel();
    LogChannel(std::string_view name, std::vector<std::shared_ptr<Sink>> p_sinks);

    template<typename... Args>
    void log(Level p_level, SourceLoc p_loc, std::format_string<Args...> p_fmt, Args &&...p_args) {
        if (p_level < level)
            return;

        LogMsg msg;
        msg.channel = name;
        msg.level = p_level;
        msg.loc = p_loc;
        msg.payload = std::format(p_fmt, std::forward<Args>(p_args)...);

        for (auto &sink: sinks) {
            if (sink->should_log(p_level))
                sink->write(msg);
        }
    }

    void flush();
    void set_level(Level p_level);

private:
    std::string name;
    Level level = Level::Trace;
    std::vector<std::shared_ptr<Sink>> sinks{};
};

} // namespace ncore::log
