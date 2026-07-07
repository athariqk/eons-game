#include <memory>
#include <vector>

#include <utils/logger/log_channel.h>

namespace nc::log {

LogChannel::LogChannel()
{
    name = DEFAULT;
}

LogChannel::LogChannel( std::string_view p_name, std::vector<std::shared_ptr<Sink>> p_sinks ) :
    name( p_name ), sinks( p_sinks )
{}

void LogChannel::flush()
{
    for (auto& sink : sinks)
        sink->flush();
}

void LogChannel::set_level( Level p_level )
{
    level = p_level;
}

} // namespace nc::log
