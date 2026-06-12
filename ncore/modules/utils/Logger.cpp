#include "Logger.h"
#include <algorithm>
#include <iostream>

namespace ncore {

LogManager::LoggerMap LogManager::m_loggers;
std::vector<spdlog::sink_ptr> LogManager::m_sinks;
bool LogManager::m_registered = false;

void LogManager::apply_pattern(Logger logger, const std::string &pattern, std::optional<std::string> pattern_tty) {
    for (auto &sink: logger->sinks()) {
        auto formatter = std::make_unique<spdlog::pattern_formatter>();
        formatter->add_flag<ShortLFormatter>('q');
        if (std::dynamic_pointer_cast<spdlog::sinks::stdout_color_sink_mt>(sink) && pattern_tty.has_value()) {
            formatter->set_pattern(*pattern_tty);
        } else {
            formatter->set_pattern(pattern);
        }
        sink->set_formatter(std::move(formatter));
    }
}

void LogManager::ShortLFormatter::format(const spdlog::details::log_msg &msg, const std::tm &,
                                         spdlog::memory_buf_t &dest) {
    std::string_view custom_level;
    switch (msg.level) {
        case spdlog::level::trace:
            custom_level = "TRC";
            break;
        case spdlog::level::debug:
            custom_level = "DBG";
            break;
        case spdlog::level::info:
            custom_level = "INF";
            break;
        case spdlog::level::warn:
            custom_level = "WRN";
            break;
        case spdlog::level::err:
            custom_level = "ERR";
            break;
        case spdlog::level::critical:
            custom_level = "FTL";
            break;
        default:
            custom_level = "OFF";
            break;
    }
    dest.append(custom_level.data(), custom_level.data() + custom_level.size());
}

std::unique_ptr<spdlog::custom_flag_formatter> LogManager::ShortLFormatter::clone() const {
    return spdlog::details::make_unique<ShortLFormatter>();
}

LogManager::InitResult LogManager::init(const std::string filePath, log::Level level) {
    try {
        m_sinks = {
            std::make_shared<spdlog::sinks::stdout_color_sink_mt>(),
            std::make_shared<spdlog::sinks::rotating_file_sink_mt>(filePath, 1024 * 1024 * 5, 3),
        };
        spdlog::set_level(to_spd_level(level));
        m_registered = true;
    } catch (const std::exception &ex) {
        return std::string(ex.what());
    }
    return std::nullopt;
}

LogManager::Logger LogManager::get(std::string_view log_name) {
    auto key = std::string(log_name);
    std::transform(key.begin(), key.end(), key.begin(), [](unsigned char c) { return std::toupper(c); });
    auto it = m_loggers.find(key);
    if (it != m_loggers.end())
        return it->second;

    if (!m_registered)
        return nullptr;

    auto logger = std::make_shared<spdlog::logger>(key, m_sinks.begin(), m_sinks.end());
    spdlog::register_logger(logger);
    m_loggers[key] = logger;
    set_level(key, from_spd_level(spdlog::get_level()));
    return logger;
}

std::string LogManager::get_level_name() { return spdlog::level::to_string_view(spdlog::get_level()).data(); }

void LogManager::shutdown() {
    for (auto &[name, logger]: m_loggers)
        logger->flush();
    spdlog::drop_all();
    m_loggers.clear();
}

void LogManager::set_level(std::string_view log_name, log::Level level) {
    auto logger = get(log_name);
    auto spd_level = to_spd_level(level);
    logger->set_level(spd_level);

    switch (spd_level) {
        case spdlog::level::trace:
            apply_pattern(logger, log::pattern_with_loc, log::pattern_with_loc_tty);
            break;
        case spdlog::level::debug:
            apply_pattern(logger, log::pattern_no_loc);
            break;
        case spdlog::level::info:
        case spdlog::level::warn:
            apply_pattern(logger, log::pattern_no_prefix);
            break;
        case spdlog::level::err:
        case spdlog::level::critical:
            apply_pattern(logger, log::pattern_with_loc, log::pattern_with_loc_tty);
            break;
        default:
            break;
    }
}

spdlog::level::level_enum LogManager::to_spd_level(log::Level level) {
    // we can do this as our enum maps 1-to-1 with spdlog
    return spdlog::level::level_enum(int(level));
}

log::Level LogManager::from_spd_level(spdlog::level::level_enum level) {
    // we can do this as our enum maps 1-to-1 with spdlog
    return log::Level(int(level));
}

} // namespace ncore
