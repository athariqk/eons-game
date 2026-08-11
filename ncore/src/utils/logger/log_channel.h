#pragma once

#include <format>

#include "sink.h"

namespace nc::log {

class Sink;

class LogChannel : public RefCounted {
    NCLASS( LogChannel, RefCounted )

public:
    LogChannel();
    LogChannel( StringView name, DynamicArray<Ref<Sink>> p_sinks );

    /**
	* @brief Push a new log message to this channel, respecting log level.
	*/
    void write( const LogMsg& msg );

    template<typename... Args>
    void write( Level p_level, SourceLoc p_loc, std::format_string<Args...> p_fmt, Args&&... p_args )
    {
        LogMsg msg{};
        msg.channel = name;
        msg.level   = p_level;
        msg.loc     = p_loc;
        msg.payload = std::vformat( p_fmt.get(), std::make_format_args( p_args... ) );
        write( msg );
    }

    void flush();
    void set_level( Level p_level );

    void set_callback( void ( *p_cb )( const LogMsg& ) );

private:
    String name;
    Level level = Level::LTRACE;
    DynamicArray<Ref<Sink>> sinks{};
    void ( *callback )( const LogMsg& );
};

} // namespace nc::log
