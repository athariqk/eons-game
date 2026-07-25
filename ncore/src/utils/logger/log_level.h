#pragma once

namespace nc::log {

enum class Level : uint8_t {
    TRACE = 0,
    DEBUG,
    INFO,
    WARN,
    ERROR,
    FATAL,
    OFF
};

struct SourceLoc {
    const char* file = nullptr;
    const char* func = nullptr;
    int line         = 0;
    bool empty() const
    {
        return file == nullptr;
    }
};

struct LogMsg {
    std::string_view channel;
    Level level = Level::OFF;
    SourceLoc loc;
    std::string payload;
};

constexpr std::string_view level_triplet( Level l )
{
    switch (l) {
        case Level::TRACE:
            return "TRC";
        case Level::DEBUG:
            return "DBG";
        case Level::INFO:
            return "INF";
        case Level::WARN:
            return "WRN";
        case Level::ERROR:
            return "ERR";
        case Level::FATAL:
            return "FAT";
        default:
            return "OFF";
    }
}

constexpr std::string_view level_name( Level l )
{
    switch (l) {
        case Level::TRACE:
            return "TRACE";
        case Level::DEBUG:
            return "DEBUG";
        case Level::INFO:
            return "INFO";
        case Level::WARN:
            return "WARN";
        case Level::ERROR:
            return "ERROR";
        case Level::FATAL:
            return "FATAL";
        default:
            return "OFF";
    }
}

constexpr std::string_view level_color( Level l )
{
    switch (l) {
        case Level::TRACE:
            return "\x1b[2;37m";
        case Level::DEBUG:
            return "\x1b[36m";
        case Level::INFO:
            return "\x1b[32m";
        case Level::WARN:
            return "\x1b[33m";
        case Level::ERROR:
            return "\x1b[31m";
        case Level::FATAL:
            return "\x1b[1;31m";
        default:
            return "";
    }
}

} // namespace nc::log
