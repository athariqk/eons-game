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

namespace Aeon {

namespace Log {
inline constexpr std::string_view Engine = "Engine";
inline constexpr std::string_view Physics = "Physics";
inline constexpr std::string_view ECS = "ECS";
inline constexpr std::string_view Game = "Game";
inline constexpr std::string_view Gui = "Gui";
inline constexpr std::string_view Graphics = "Graphics";
inline constexpr std::string_view Audio = "Audio";

enum Level { Trace = 0, Debug, Info, Warn, Error, Critical, Off };
} // namespace Log

class Logger {
public:
    static std::optional<std::string> Init(const std::string filePath);
    static void Shutdown();

    static void SetLevel(std::string_view cat, std::string_view level);

    static std::shared_ptr<spdlog::logger> Get(std::string_view name);

private:
    static std::unordered_map<std::string, std::shared_ptr<spdlog::logger>> m_loggers;
    static std::vector<spdlog::sink_ptr> m_sinks;
    static bool m_registered;
};

} // namespace Aeon

#define LOG_TRACE(cat, ...) SPDLOG_LOGGER_TRACE(Aeon::Logger::Get(cat), __VA_ARGS__)
#define LOG_DEBUG(cat, ...) SPDLOG_LOGGER_DEBUG(Aeon::Logger::Get(cat), __VA_ARGS__)
#define LOG_INFO(cat, ...) SPDLOG_LOGGER_INFO(Aeon::Logger::Get(cat), __VA_ARGS__)
#define LOG_WARN(cat, ...) SPDLOG_LOGGER_WARN(Aeon::Logger::Get(cat), __VA_ARGS__)
#define LOG_ERROR(cat, ...) SPDLOG_LOGGER_ERROR(Aeon::Logger::Get(cat), __VA_ARGS__)
#define LOG_FATAL(cat, ...) SPDLOG_LOGGER_FATAL(Aeon::Logger::Get(cat), __VA_ARGS__)
