#include <algorithm>

#include <ncore/modules/events/event_bus.h>

namespace nc {

Error EventBus::init( ConfFile& cfg_file )
{
    return Error::OK;
}

void EventBus::finalize()
{
    clear();
}

void EventBus::unsubscribe( size_t p_subscription_id )
{
    for (auto& [ev, subs] : subscribers) {
        subs.erase(
            std::remove_if(
                subs.begin(), subs.end(),
                [p_subscription_id]( const auto& sub ) { return sub.first == p_subscription_id; }
            ),
            subs.end()
        );
    }
}

void EventBus::flush()
{
    for (auto event : event_queue) {
        auto it = subscribers.find( event->get_type_id() );
        if (it == subscribers.end())
            continue;

        for (auto& [id, callback] : it->second) {
            callback( *event );
            if (event->handled)
                break;
        }
    }

    event_queue.clear();
}

size_t EventBus::get_queue_size() const
{
    return event_queue.size();
}

EventBus::SubscriberDebugInfo EventBus::get_subscriber_debug_info() const
{
    SubscriberDebugInfo info;
    for (const auto& [event_type, subs] : subscribers)
        info.emplace_back( event_type, subs.size() );
    return info;
}

void EventBus::clear()
{
    subscribers.clear();
    event_queue.clear();
    next_subscription_id = 0;
}

} // namespace nc
