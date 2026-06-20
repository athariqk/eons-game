#pragma once

#include <format>

#include "sink.h"

namespace ncore::log {

class Sink;

class LogChannel {
public:
    LogChannel();
    LogChannel(std::string_view name, std::vector<std::shared_ptr<Sink>> p_sinks);

    template<typename... Args>
    void write(Level p_level, SourceLoc p_loc, std::format_string<Args...> p_fmt, Args &&...p_args) {
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
