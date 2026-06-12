#pragma once

// From Kass Engine

#include <optional>
#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>

#define SPDLOG_ACTIVE_LEVEL SPDLOG_LEVEL_TRACE
#include "spdlog/pattern_formatter.h"
#include "spdlog/sinks/rotating_file_sink.h"
#include "spdlog/sinks/stdout_color_sinks.h"
#include "spdlog/spdlog.h"

namespace ncore {

namespace log {
inline constexpr std::string_view ENGINE = "engine";
inline constexpr std::string_view PHYSICS = "phys";
inline constexpr std::string_view ECS = "ecs";
inline constexpr std::string_view GAME = "game";
inline constexpr std::string_view GUI = "gui";
inline constexpr std::string_view GRAPHICS = "graphs";
inline constexpr std::string_view AUDIO = "audio";

inline constexpr auto pattern_with_loc_tty = "[%n] %T - %^%q%$: %v\n\x1b[2;37mloc: %@\x1b[0m\n";
inline constexpr auto pattern_with_loc = "[%n] %T - %^%q%$: %v\nloc: %@\n";
inline constexpr auto pattern_no_loc = "[%n] %T - %^%q%$: %v";
inline constexpr auto pattern_no_prefix = "%T - %^%q%$: %v";

enum class Level : int { Trace = 0, Debug, Info, Warn, Error, Critical, Off };

} // namespace log

class LogManager {
public:
    using Logger = std::shared_ptr<spdlog::logger>;
    using InitResult = std::optional<std::string>;

    static InitResult init(const std::string filePath, log::Level level);
    static void shutdown();
    static void set_level(std::string_view log_name, log::Level level);
    static Logger get(std::string_view log_name);

    // Gets the global log level name as string
    static std::string get_level_name();

private:
    using LoggerMap = std::unordered_map<std::string, Logger>;

    class ShortLFormatter : public spdlog::custom_flag_formatter {
    public:
        void format(const spdlog::details::log_msg &msg, const std::tm &, spdlog::memory_buf_t &dest) override;
        std::unique_ptr<spdlog::custom_flag_formatter> clone() const override;
    };

    static void apply_pattern(Logger logger, const std::string &pattern,
                              std::optional<std::string> pattern_tty = std::nullopt);
    static spdlog::level::level_enum to_spd_level(log::Level level);
    static log::Level from_spd_level(spdlog::level::level_enum level);

    static LoggerMap m_loggers;
    static std::vector<spdlog::sink_ptr> m_sinks;
    static bool m_registered;
};

} // namespace ncore

#define LOG_TRACE(cat, ...) SPDLOG_LOGGER_TRACE(ncore::LogManager::get(cat), __VA_ARGS__)
#define LOG_DEBUG(cat, ...) SPDLOG_LOGGER_DEBUG(ncore::LogManager::get(cat), __VA_ARGS__)
#define LOG_INFO(cat, ...) SPDLOG_LOGGER_INFO(ncore::LogManager::get(cat), __VA_ARGS__)
#define LOG_WARN(cat, ...) SPDLOG_LOGGER_WARN(ncore::LogManager::get(cat), __VA_ARGS__)
#define LOG_ERROR(cat, ...) SPDLOG_LOGGER_ERROR(ncore::LogManager::get(cat), __VA_ARGS__)
#define LOG_FATAL(cat, ...) SPDLOG_LOGGER_CRITICAL(ncore::LogManager::get(cat), __VA_ARGS__)
