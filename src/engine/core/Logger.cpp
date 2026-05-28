#include "Logger.h"

#include "spdlog/sinks/stdout_sinks.h"

std::shared_ptr<spdlog::logger> Logger::Log;

void Logger::Init() {
    spdlog::set_pattern("%^[%T] %n: %v%$");
    Log = spdlog::stdout_logger_mt("Application");
    Log->set_level(spdlog::level::trace);
}
