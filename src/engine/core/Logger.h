#pragma once

// From Kass Engine
#define SPDLOG_ACTIVE_LEVEL SPDLOG_LEVEL_TRACE
#include "spdlog/sinks/stdout_sinks.h"
#include "spdlog/spdlog.h"

namespace Aeon {

class Logger {
public:
    static void Init();

    inline static std::shared_ptr<spdlog::logger> &GetEngineLogger() { return m_engineLogger; }

private:
    static std::shared_ptr<spdlog::logger> m_engineLogger;
};

} // namespace Aeon

// Log macros
#define LOG_TRACE(...) SPDLOG_LOGGER_TRACE(Aeon::Logger::GetEngineLogger(), __VA_ARGS__)
#define LOG_DEBUG(...) SPDLOG_LOGGER_DEBUG(Aeon::Logger::GetEngineLogger(), __VA_ARGS__)
#define LOG_INFO(...) SPDLOG_LOGGER_INFO(Aeon::Logger::GetEngineLogger(), __VA_ARGS__)
#define LOG_WARN(...) SPDLOG_LOGGER_WARN(Aeon::Logger::GetEngineLogger(), __VA_ARGS__)
#define LOG_ERROR(...) SPDLOG_LOGGER_ERROR(Aeon::Logger::GetEngineLogger(), __VA_ARGS__)
#define LOG_FATAL(...) SPDLOG_LOGGER_FATAL(Aeon::Logger::GetEngineLogger(), __VA_ARGS__)
