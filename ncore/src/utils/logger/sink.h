#pragma once

#include <fstream>
#include <mutex>
#include <string>

#include "log_level.h"

namespace nc::log {

class Sink {
public:
    virtual ~Sink();

    virtual void write( const LogMsg& msg ) = 0;
    virtual void flush()                    = 0;

    void set_level( Level p_level )
    {
        level = p_level;
    }
    bool should_log( Level p_level ) const
    {
        return p_level >= level;
    }

protected:
    std::string current_time();

private:
    Level level = Level::Trace;
};

class ConsoleSink : public Sink {
public:
    void write( const LogMsg& msg ) override;
    void flush() override;
};

class FileSink : public Sink {
public:
    FileSink( const std::string& path, size_t max_bytes = 1024 * 1024 * 5, size_t max_files = 3 );

    void write( const LogMsg& msg ) override;
    void flush() override;

private:
    void rotate();
    void open();

    std::ofstream m_file;
    std::string m_path;
    std::mutex m_mutex;
    size_t m_max_bytes;
    size_t m_max_files;
    size_t m_bytes_written = 0;
};

} // namespace nc::log
