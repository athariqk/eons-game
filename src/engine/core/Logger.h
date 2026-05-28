#pragma once

// From Kass Engine
#include "spdlog/spdlog.h"

class Logger {
public:
    static void Init();

    inline static std::shared_ptr<spdlog::logger> &GetLogger() { return Log; }

private:
    static std::shared_ptr<spdlog::logger> Log;
};

// Log macros
#define LOG_TRACE(...) Logger::GetLogger()->trace(__VA_ARGS__)
#define LOG_DEBUG(...) Logger::GetLogger()->debug(__VA_ARGS__)
#define LOG_INFO(...) Logger::GetLogger()->info(__VA_ARGS__)
#define LOG_WARN(...) Logger::GetLogger()->warn(__VA_ARGS__)
#define LOG_ERROR(...) Logger::GetLogger()->error(__VA_ARGS__)
#define LOG_FATAL(...) Logger::GetLogger()->fatal(__VA_ARGS__)
