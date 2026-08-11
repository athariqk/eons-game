#pragma once

#include <fstream>
#include <mutex>

#include <ncore/core/reference.h>
#include <ncore/utils/log.h>

namespace nc::log {

/**
* @brief Output target for log messages.
*/
class Sink : public RefCounted {
    NCLASS( Sink, RefCounted )

public:
    virtual ~Sink();

    /**
	* @brief Push a new log message, respecting log level.
	*/
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
    String current_time();

private:
    Level level = Level::LTRACE;
};

class ConsoleSink : public Sink {
    NCLASS( ConsoleSink, Sink )

public:
    void write( const LogMsg& msg ) override;
    void flush() override;
};

class FileSink : public Sink {
    NCLASS( FileSink, Sink )

public:
    FileSink( const String& path, size_t max_bytes = 1024 * 1024 * 5, size_t max_files = 3 );

    void write( const LogMsg& msg ) override;
    void flush() override;

private:
    void rotate();
    void open();

    std::ofstream m_file;
    String m_path;
    std::mutex m_mutex;
    size_t m_max_bytes;
    size_t m_max_files;
    size_t m_bytes_written = 0;
};

} // namespace nc::log
