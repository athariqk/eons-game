#pragma once

#include <functional>
#include <memory>
#include <typeindex>
#include <unordered_map>
#include <vector>

namespace ncore {

/**
 * @brief Base class for all events
 *
 * Events are lightweight data structures that represent something
 * that happened in the game (input, collision, gameplay event, etc.)
 */
class Event {
public:
    virtual ~Event() = default;

    // Mark event as handled to prevent further processing
    mutable bool handled = false;
};

class UnknownEvent : public Event {
public:
    UnknownEvent(void *p_native_ptr) : native_ptr(p_native_ptr) {}
    void *native_ptr;
};

/**
 * @brief Type-safe event bus for publishing and subscribing to events
 *
 * The EventBus allows systems to subscribe to specific event types
 * and receive callbacks when those events are published.
 *
 * Usage:
 *   // Subscribe
 *   event_bus.subscribe<CollisionEvent>([](const CollisionEvent &e) {
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
     * The order of subscribed consumers defines the event pipeline
     *
     * @tparam T Event type to subscribe to
     * @param callback Function to call when event is published
     * @return Subscription ID (can be used to unsubscribe)
     */
    template<typename T>
    size_t subscribe(std::function<void(const T &)> callback) {
        static_assert(std::is_base_of_v<Event, T>, "T must inherit from Event");

        auto type_idx = std::type_index(typeid(T));
        size_t id = next_subscription_id++;

        // Wrap the typed callback in a type-erased wrapper
        auto wrapper = [callback](const Event &event) { callback(static_cast<const T &>(event)); };

        subscribers[type_idx].emplace_back(id, wrapper);
        return id;
    }

    /**
     * @brief Unsubscribe from an event
     * @param subscriptionId ID returned from subscribe()
     */
    void unsubscribe(size_t p_subscription_id) {
        for (auto &[type_idx, subscribers]: subscribers) {
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
    template<typename T>
    void publish(const T &event) {
        static_assert(std::is_base_of_v<Event, T>, "T must inherit from Event");

        auto type_idx = std::type_index(typeid(T));
        auto it = subscribers.find(type_idx);
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
    template<typename T>
    void queue(const T &event) {
        static_assert(std::is_base_of_v<Event, T>, "T must inherit from Event");
        event_queue.emplace_back(std::make_shared<T>(event));
    }

    /**
     * @brief Process all queued events
     *
     * Called once per frame to process events that were queued
     * using Queue(). Events are processed in FIFO order.
     */
    void process_queue() {
        // Process queued events
        for (auto &event: event_queue) {
            auto type_idx = std::type_index(typeid(*event));
            auto it = subscribers.find(type_idx);
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

    using SubscriberDebugInfo = std::vector<std::pair<const char *, size_t>>;
    SubscriberDebugInfo get_subscriber_debug_info() const {
        SubscriberDebugInfo info;
        for (const auto &[typeIndex, subs]: subscribers)
            info.emplace_back(typeIndex.name(), subs.size());
        return info;
    }

private:
    // Type index -> list of (subscription ID, callback)
    using CallbackType = std::function<void(const Event &)>;
    std::unordered_map<std::type_index, std::vector<std::pair<size_t, CallbackType>>> subscribers;

    // Queue for deferred event processing
    std::vector<std::shared_ptr<Event>> event_queue;

    size_t next_subscription_id = 0;
};

} // namespace ncore
