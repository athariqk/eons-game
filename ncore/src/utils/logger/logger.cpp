#include <utils/logger/logger.h>

#include <ncore/utils/log.h>

namespace ncore::log {

Logger::Logger() { add_sink(std::make_shared<log::ConsoleSink>()); }

void Logger::add_sink(std::shared_ptr<Sink> p_sink) { global_sinks.push_back(p_sink); }

std::shared_ptr<LogChannel> Logger::channel(std::string_view p_channel) {
    auto key = std::string(p_channel);
    std::transform(key.begin(), key.end(), key.begin(), [](unsigned char c) { return std::toupper(c); });
    auto it = channels.find(key);
    if (it != channels.end())
        return it->second;

    auto logger = std::make_shared<LogChannel>(key, global_sinks);
    logger->set_level(global_level);
    channels[key] = logger;
    return logger;
}

void Logger::set_level(std::string_view log_name, log::Level level) {
    auto c = channel(log_name);
    c->set_level(level);
    NC_LOG_TRACE("Log level for channel '{}' set to: {}", log_name, int(level));
}

Level Logger::get_level() const { return global_level; }

void Logger::set_level(log::Level level) {
    global_level = level;
    for (auto &[name, ch]: channels)
        ch->set_level(level);
    NC_LOG_TRACE("Global log level set to: {}", int(global_level));
}

void Logger::flush_all() {
    for (auto &[name, ch]: channels)
        ch->flush();
}

} // namespace ncore::log
