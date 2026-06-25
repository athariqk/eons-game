#include <algorithm>

#include <ncore/modules/events/event_bus.h>

namespace ncore {

void EventBus::unsubscribe(size_t p_subscription_id)
{
    for (auto& [event_type, subscribers] : subscribers) {
        subscribers.erase(
            std::remove_if(
                subscribers.begin(), subscribers.end(),
                [p_subscription_id](const auto& sub) { return sub.first == p_subscription_id; }
            ),
            subscribers.end()
        );
    }
}

void EventBus::process_queue()
{
    for (auto& event : event_queue) {
        auto it = subscribers.find(event->get_type_id());
        if (it == subscribers.end())
            continue;

        for (auto& [id, callback] : it->second) {
            callback(*event);
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
        info.emplace_back(event_type, subs.size());
    return info;
}

void EventBus::finalize()
{
    subscribers.clear();
    event_queue.clear();
    next_subscription_id = 0;
}

} // namespace ncore
