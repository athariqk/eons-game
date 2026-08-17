#include <utils/logger/logger.h>

namespace nc::log {

Logger::Logger()
{
    add_sink( Ref<ConsoleSink>::create() );
}

void Logger::add_sink( const Ref<Sink>& p_sink )
{
    //NC_LOG_DEBUG( "Adding global sink '{}'", p_sink->get_class_name() );
    global_sinks.push_back( p_sink );
}

Ref<LogChannel> Logger::channel( StringView p_channel )
{
    auto key = String( p_channel );
    std::transform( key.begin(), key.end(), key.begin(), []( unsigned char c ) { return std::toupper( c ); } );
    auto it = channels.find( key );
    if (it != channels.end())
        return it->second;

    auto logger = Ref<LogChannel>::create( key, global_sinks );
    logger->set_level( global_level );
    channels[key] = logger;

    // forward message to listeners.
    logger->set_callback( []( const LogMsg& msg ) {
        auto& cbs = Logger::get_instance().callbacks;
        auto iter = cbs.begin();
        while (iter != cbs.end()) {
            if (auto fn = iter->lock()) {
                ( *fn )( msg );
                ++iter;
            } else {
                iter = cbs.erase( iter );
            }
        }
    } );

    return logger;
}

void Logger::set_level( StringView p_channel, log::Level level )
{
    auto c = channel( p_channel );
    c->set_level( level );
    NC_LOG_TRACE( "Log level for channel '{}' set to: {}", p_channel, int( level ) );
}

Level Logger::get_level() const
{
    return global_level;
}

void Logger::set_level( log::Level level )
{
    global_level = level;
    for (auto& [name, ch] : channels)
        ch->set_level( level );
    NC_LOG_TRACE( "Global log level set to: {}", int( global_level ) );
}

void Logger::flush_all()
{
    for (auto& [name, ch] : channels)
        ch->flush();
}

ListenerToken Logger::add_listener( const LogMsgCallback& callback )
{
    auto token = std::make_shared<LogMsgCallback>( callback );
    callbacks.push_back( token );
    return token;
}

} // namespace nc::log
