#pragma once

#include <functional>
#include <memory>
#include <typeindex>
#include <unordered_map>
#include <vector>

#include <modules/events/Event.h>

namespace ncore {

/**
 * @brief Type-safe event bus for publishing and subscribing to events
 *
 * The EventBus allows systems to subscribe to specific event types
 * and receive callbacks when those events are published.
 *
 * Usage:
 *   // Subscribe
 *   event_bus.subscribe<CollisionEvent>([](CollisionEvent &e) {
 *       // Handle collision
 *   });
 *
 *   // Publish
 *   event_bus.publish(CollisionEvent(entityA, entityB));
 */
class EventBus {
public:
    /**
     * @brief subscribe to an event type
     *
     * The order of subscribed consumers defines the event pipeline.
     * Subscribing to the base Event type is not supported.
     *
     * @tparam T Event type to subscribe to
     * @param callback Function to call when event is published
     * @return Subscription ID (can be used to unsubscribe)
     */
    template<std::derived_from<Event> T>
    size_t subscribe(std::function<void(T &)> callback) {
        size_t id = next_subscription_id++;
        auto wrapper = [callback](Event &event) { callback(static_cast<T &>(event)); };
        subscribers[T::static_type].emplace_back(id, wrapper);
        return id;
    }

    /**
     * @brief Unsubscribe from an event
     * @param subscriptionId ID returned from subscribe()
     */
    void unsubscribe(size_t p_subscription_id) {
        for (auto &[event_type, subscribers]: subscribers) {
            subscribers.erase(
                std::remove_if(subscribers.begin(), subscribers.end(),
                               [p_subscription_id](const auto &sub) { return sub.first == p_subscription_id; }),
                subscribers.end());
        }
    }

    /**
     * @brief Publish an event immediately (synchronous)
     * @tparam T Event type
     * @param event Event instance to publish
     */
    template<std::derived_from<Event> T>
    void publish(const T &event) {
        auto it = subscribers.find(event.get_type());
        if (it == subscribers.end())
            return;

        for (auto &[id, callback]: it->second) {
            callback(event);
            if (event.handled)
                break; // Stop propagation if marked as handled
        }
    }

    /**
     * @brief Queue an event for deferred processing
     * @tparam T Event type
     * @param event Event instance to queue
     *
     * Queued events are processed when ProcessQueue() is called.
     * Useful for events that should not be processed immediately.
     */
    template<std::derived_from<Event> T>
    void enqueue(std::unique_ptr<T> event) {
        event_queue.emplace_back(std::move(event));
    }

    /**
     * @brief Process all queued events
     *
     * Called once per frame to process events that were queued
     * using Queue(). Events are processed in FIFO order.
     */
    void process_queue() {
        for (auto &event: event_queue) {
            auto it = subscribers.find(event->get_type());
            if (it == subscribers.end())
                continue;

            for (auto &[id, callback]: it->second) {
                callback(*event);
                if (event->handled)
                    break;
            }
        }

        event_queue.clear();
    }

    void clear() {
        subscribers.clear();
        event_queue.clear();
        next_subscription_id = 0;
    }

    // ---- Read-only debug accessors ----

    size_t get_queue_size() const { return event_queue.size(); }

    using SubscriberDebugInfo = std::vector<std::pair<EventType, size_t>>;
    SubscriberDebugInfo get_subscriber_debug_info() const {
        SubscriberDebugInfo info;
        for (const auto &[event_type, subs]: subscribers)
            info.emplace_back(event_type, subs.size());
        return info;
    }

private:
    using CallbackType = std::function<void(Event &)>;
    using SubscriberList = std::vector<std::pair<size_t, CallbackType>>;
    std::unordered_map<EventType, SubscriberList> subscribers;

    // We own the queued events
    std::vector<std::unique_ptr<Event>> event_queue;

    size_t next_subscription_id = 0;
};

} // namespace ncore
