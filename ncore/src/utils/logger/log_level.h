#pragma once

namespace ncore::log {

enum class Level : uint8_t { Trace = 0, Debug, Info, Warn, Error, Fatal, Off };

struct SourceLoc {
    const char *file = nullptr;
    const char *func = nullptr;
    int line = 0;
    bool empty() const { return file == nullptr; }
};

struct LogMsg {
    std::string_view channel;
    Level level = Level::Off;
    SourceLoc loc;
    std::string payload;
};

constexpr std::string_view level_triplet(Level l) {
    switch (l) {
        case Level::Trace:
            return "TRC";
        case Level::Debug:
            return "DBG";
        case Level::Info:
            return "INF";
        case Level::Warn:
            return "WRN";
        case Level::Error:
            return "ERR";
        case Level::Fatal:
            return "FAT";
        default:
            return "OFF";
    }
}

constexpr std::string_view level_name(Level l) {
    switch (l) {
        case Level::Trace:
            return "TRACE";
        case Level::Debug:
            return "DEBUG";
        case Level::Info:
            return "INFO";
        case Level::Warn:
            return "WARN";
        case Level::Error:
            return "ERROR";
        case Level::Fatal:
            return "FATAL";
        default:
            return "OFF";
    }
}

constexpr std::string_view level_color(Level l) {
    switch (l) {
        case Level::Trace:
            return "\x1b[2;37m";
        case Level::Debug:
            return "\x1b[36m";
        case Level::Info:
            return "\x1b[32m";
        case Level::Warn:
            return "\x1b[33m";
        case Level::Error:
            return "\x1b[31m";
        case Level::Fatal:
            return "\x1b[1;31m";
        default:
            return "";
    }
}

} // namespace ncore::log
