#pragma once

#include <unordered_map>

#include "LogChannel.h"
#include "LogLevel.h"

namespace ncore::log {

class Logger {
public:
    Logger(const Logger &) = delete;
    Logger &operator=(const Logger &) = delete;

    Logger(Logger &&) = delete;
    Logger &operator=(Logger &&) = delete;

    static Logger &get_instance() {
        static Logger instance;
        return instance;
    }

    // Add to the global sinks for all log channels
    void add_sink(std::shared_ptr<Sink> p_sink);

    std::shared_ptr<LogChannel> get_or_create(std::string_view p_channel = DEFAULT);
    void set_level(std::string_view log_name, log::Level level);

    // Gets and sets the global log level
    Level get_level() const;
    void set_level(log::Level level);

    void flush_all();

private:
    Logger();

    Level global_level = Level::Trace;
    std::vector<std::shared_ptr<Sink>> global_sinks{};
    std::unordered_map<std::string, std::shared_ptr<LogChannel>> channels{};
};

} // namespace ncore::log
