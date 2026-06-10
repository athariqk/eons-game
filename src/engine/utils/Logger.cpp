#include "Logger.h"

#include <filesystem>

namespace ncore {

std::unordered_map<std::string, std::shared_ptr<spdlog::logger>> Logger::m_loggers;
std::vector<spdlog::sink_ptr> Logger::m_sinks;
bool Logger::m_registered = false;

std::shared_ptr<spdlog::logger> Logger::get(std::string_view name) {
    auto key = std::string(name);
    auto it = m_loggers.find(key);
    if (it != m_loggers.end())
        return it->second;

    if (!m_registered)
        return nullptr;

    auto logger = std::make_shared<spdlog::logger>(key, m_sinks.begin(), m_sinks.end());

#ifdef DEBUG
    logger->set_pattern("%^[%T] [%s:%#] %v%$");
#else
    logger->set_pattern("%^[%T] [%n] %v%$");
#endif // DEBUG

    logger->set_level(spdlog::level::trace);
    spdlog::register_logger(logger);
    m_loggers[key] = logger;
    return logger;
}

std::optional<std::string> Logger::init(const std::string filePath) {
    try {
        m_sinks = {
            std::make_shared<spdlog::sinks::stdout_color_sink_mt>(),
            std::make_shared<spdlog::sinks::rotating_file_sink_mt>(filePath, 1024 * 1024 * 5, 3),
        };
        m_registered = true;
    } catch (const std::exception &ex) {
        return std::string(ex.what());
    }

    return std::nullopt;
}

void Logger::shutdown() {
    for (auto &[name, logger]: m_loggers)
        logger->flush();
    spdlog::drop_all();
    m_loggers.clear();
}

void Logger::set_level(std::string_view cat, std::string_view level) {
    get(cat)->set_level(spdlog::level::from_str(level.data()));
}

} // namespace ncore
