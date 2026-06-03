#include "Logger.h"

namespace Aeon {

std::shared_ptr<spdlog::logger> Logger::m_engineLogger;

void Logger::Init() {
    m_engineLogger = spdlog::stdout_logger_mt("Engine");
    m_engineLogger->set_pattern("%^[%T] [%!:%#] %v%$");
    m_engineLogger->set_level(spdlog::level::trace);
    LOG_TRACE("Engine log initialized");
}

} // namespace Aeon
