#pragma once

// From Kass Engine

#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>

#define SPDLOG_ACTIVE_LEVEL SPDLOG_LEVEL_TRACE
#include "spdlog/sinks/rotating_file_sink.h"
#include "spdlog/sinks/stdout_color_sinks.h"
#include "spdlog/spdlog.h"

namespace ncore {

namespace log {
inline constexpr std::string_view ENGINE = "Engine";
inline constexpr std::string_view PHYSICS = "Physics";
inline constexpr std::string_view ECS = "ECS";
inline constexpr std::string_view GAME = "Game";
inline constexpr std::string_view GUI = "Gui";
inline constexpr std::string_view GRAPHICS = "Graphics";
inline constexpr std::string_view AUDIO = "Audio";

enum Level { Trace = 0, Debug, Info, Warn, Error, Critical, Off };
} // namespace log

class Logger {
public:
    static std::optional<std::string> init(const std::string filePath);
    static void shutdown();

    static void set_level(std::string_view cat, std::string_view level);

    static std::shared_ptr<spdlog::logger> get(std::string_view name);

private:
    static std::unordered_map<std::string, std::shared_ptr<spdlog::logger>> m_loggers;
    static std::vector<spdlog::sink_ptr> m_sinks;
    static bool m_registered;
};

} // namespace ncore

#define LOG_TRACE(cat, ...) SPDLOG_LOGGER_TRACE(ncore::Logger::get(cat), __VA_ARGS__)
#define LOG_DEBUG(cat, ...) SPDLOG_LOGGER_DEBUG(ncore::Logger::get(cat), __VA_ARGS__)
#define LOG_INFO(cat, ...) SPDLOG_LOGGER_INFO(ncore::Logger::get(cat), __VA_ARGS__)
#define LOG_WARN(cat, ...) SPDLOG_LOGGER_WARN(ncore::Logger::get(cat), __VA_ARGS__)
#define LOG_ERROR(cat, ...) SPDLOG_LOGGER_ERROR(ncore::Logger::get(cat), __VA_ARGS__)
#define LOG_FATAL(cat, ...) SPDLOG_LOGGER_FATAL(ncore::Logger::get(cat), __VA_ARGS__)
