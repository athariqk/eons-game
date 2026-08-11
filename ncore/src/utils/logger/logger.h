#pragma once

#include "log_channel.h"

namespace nc::log {

class Logger {
public:
    Logger( const Logger& )            = delete;
    Logger& operator=( const Logger& ) = delete;

    Logger( Logger&& )            = delete;
    Logger& operator=( Logger&& ) = delete;

    static Logger& get_instance()
    {
        static Logger instance;
        return instance;
    }

    /**
     * @brief Add to the global sinks for all log channels
     */
    void add_sink( const Ref<Sink>& p_sink );

    /**
     * @brief Get a channel to start logging messages.
     * @param p_channel Name of the channel (Default = NC).
     */
    Ref<LogChannel> channel( StringView p_channel = DEFAULT );
    /**
     * @brief Set the log level of given channel.
     */
    void set_level( StringView p_channel, log::Level level );

    /**
     * @brief Gets global log level.
     *
     * Does not affect individual channels.
     */
    Level get_level() const;
    /**
     *  @brief Sets global log level.
     *
     * Does not affect individual channels.
     */
    void set_level( log::Level level );

    /**
     * @brief Flush buffer from all channels.
     */
    void flush_all();

    ListenerToken add_listener( const LogMsgCallback& callback );

private:
    Logger();

    Level global_level = Level::LTRACE;
    DynamicArray<Ref<Sink>> global_sinks{};
    HashMap<String, Ref<LogChannel>> channels{};
    DynamicArray<std::weak_ptr<LogMsgCallback>> callbacks;
};

} // namespace nc::log
