#pragma once

#include <functional>
#include <memory>
#include <typeindex>
#include <unordered_map>
#include <vector>

namespace Aeon {

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
    bool handled = false;
};

/**
 * @brief Type-safe event bus for publishing and subscribing to events
 *
 * The EventBus allows systems to subscribe to specific event types
 * and receive callbacks when those events are published.
 *
 * Usage:
 *   // Subscribe
 *   eventBus.Subscribe<CollisionEvent>([](const CollisionEvent &e) {
 *       // Handle collision
 *   });
 *
 *   // Publish
 *   eventBus.Publish(CollisionEvent{entityA, entityB});
 */
class EventBus {
public:
    /**
     * @brief Subscribe to an event type
     * @tparam T Event type to subscribe to
     * @param callback Function to call when event is published
     * @return Subscription ID (can be used to unsubscribe)
     */
    template<typename T>
    size_t Subscribe(std::function<void(const T &)> callback) {
        static_assert(std::is_base_of_v<Event, T>, "T must inherit from Event");

        auto typeIndex = std::type_index(typeid(T));
        size_t id = m_nextSubscriptionId++;

        // Wrap the typed callback in a type-erased wrapper
        auto wrapper = [callback](const Event &event) { callback(static_cast<const T &>(event)); };

        m_subscribers[typeIndex].emplace_back(id, wrapper);
        return id;
    }

    /**
     * @brief Unsubscribe from an event
     * @param subscriptionId ID returned from Subscribe()
     */
    void Unsubscribe(size_t subscriptionId) {
        for (auto &[typeIndex, subscribers]: m_subscribers) {
            subscribers.erase(std::remove_if(subscribers.begin(), subscribers.end(),
                                             [subscriptionId](const auto &sub) { return sub.first == subscriptionId; }),
                              subscribers.end());
        }
    }

    /**
     * @brief Publish an event immediately (synchronous)
     * @tparam T Event type
     * @param event Event instance to publish
     */
    template<typename T>
    void Publish(const T &event) {
        static_assert(std::is_base_of_v<Event, T>, "T must inherit from Event");

        auto typeIndex = std::type_index(typeid(T));
        auto it = m_subscribers.find(typeIndex);
        if (it == m_subscribers.end())
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
    void Queue(const T &event) {
        static_assert(std::is_base_of_v<Event, T>, "T must inherit from Event");
        m_eventQueue.emplace_back(std::make_shared<T>(event));
    }

    /**
     * @brief Process all queued events
     *
     * Call this once per frame to process events that were queued
     * using Queue(). Events are processed in FIFO order.
     */
    void ProcessQueue() {
        // Process queued events
        for (auto &event: m_eventQueue) {
            auto typeIndex = std::type_index(typeid(*event));
            auto it = m_subscribers.find(typeIndex);
            if (it == m_subscribers.end())
                continue;

            for (auto &[id, callback]: it->second) {
                callback(*event);
                if (event->handled)
                    break;
            }
        }

        m_eventQueue.clear();
    }

    void Clear() {
        m_subscribers.clear();
        m_eventQueue.clear();
        m_nextSubscriptionId = 0;
    }

private:
    // Type index -> list of (subscription ID, callback)
    using CallbackType = std::function<void(const Event &)>;
    std::unordered_map<std::type_index, std::vector<std::pair<size_t, CallbackType>>> m_subscribers;

    // Queue for deferred event processing
    std::vector<std::shared_ptr<Event>> m_eventQueue;

    size_t m_nextSubscriptionId = 0;
};

} // namespace Aeon
