#pragma once

#include <utils/logger/sink.h>

namespace ncore::log {

Sink::~Sink() {}

std::string Sink::current_time()
{
    auto now         = std::chrono::system_clock::now();
    auto now_floored = std::chrono::floor<std::chrono::seconds>(now);
    return std::format("{:%H:%M:%S}", now_floored);
}

void ConsoleSink::write(const LogMsg& msg)
{
    std::string out = std::format(
        "\x1b[0m[{}] {} - {}{}\x1b[0m: {}\n", msg.channel, current_time(), level_color(msg.level),
        level_name(msg.level), msg.payload
    );
    if (!msg.loc.empty()) {
        out += std::format("\x1b[2;37mloc: {}:{}\x1b[0m\n\n", msg.loc.file, msg.loc.line);
    }
    fputs(out.c_str(), stderr);
}

void ConsoleSink::flush()
{
    fflush(stderr);
}

FileSink::FileSink(const std::string& path, size_t max_bytes, size_t max_files) :
    m_path(path), m_max_bytes(max_bytes), m_max_files(max_files)
{
    open();
}

void FileSink::write(const LogMsg& msg)
{
    std::lock_guard lock(m_mutex);
    auto idx  = static_cast<uint8_t>(msg.level);
    auto line = std::format("[{}] {} - {}: {}\n", msg.channel, current_time(), level_name(msg.level), msg.payload);

    if (!msg.loc.empty())
        line += std::format("loc: {}:{}\n", msg.loc.file, msg.loc.line);

    m_file << line;
    m_bytes_written += line.size();

    if (m_bytes_written >= m_max_bytes)
        rotate();
}

void FileSink::flush()
{
    std::lock_guard lock(m_mutex);
    m_file.flush();
}

void FileSink::rotate()
{
    m_file.close();
    for (int i = static_cast<int>(m_max_files) - 1; i >= 1; --i) {
        auto src  = m_path + "." + std::to_string(i);
        auto dest = m_path + "." + std::to_string(i + 1);
        std::rename(src.c_str(), dest.c_str());
    }
    std::rename(m_path.c_str(), (m_path + ".1").c_str());
    open();
}

void FileSink::open()
{
    m_file.open(m_path, std::ios::app);
    m_bytes_written = m_file.tellp();
}

} // namespace ncore::log
