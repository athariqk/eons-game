#include <utils/logger/log_channel.h>

namespace nc::log {

LogChannel::LogChannel()
{
    name = DEFAULT;
}

LogChannel::LogChannel( StringView p_name, DynamicArray<Ref<Sink>> p_sinks ) : name( p_name ), sinks( p_sinks ) {}

void LogChannel::write( const LogMsg& msg )
{
    if (msg.level < level)
        return;

    for (auto& sink : sinks) {
        if (sink->should_log( msg.level ))
            sink->write( msg );
    }

    if (callback) {
        callback( msg );
    }
}

void LogChannel::flush()
{
    for (auto& sink : sinks)
        sink->flush();
}

void LogChannel::set_level( Level p_level )
{
    level = p_level;
}

void LogChannel::set_callback( void ( *p_cb )( const LogMsg& ) )
{
    callback = p_cb;
}

} // namespace nc::log
