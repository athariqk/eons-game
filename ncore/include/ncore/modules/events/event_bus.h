#pragma once

#include <functional>
#include <memory>

#include <ncore/modules/module.h>

#include "events.h"

namespace nc {

/**
 * @brief EventBus is an implementation of the event bus pattern
 * for publishing and subscribing to Events.
 *
 * Usage:
 *   // Subscribe
 *   events.subscribe<CollisionEvent>([](CollisionEvent &e) {
 *       // Handle collision
 *   });
 *
 *   // Publish
 *   events.publish(CollisionEvent(entityA, entityB));
 */
class NCORE_API EventBus : public IModule {
    NCLASS( EventBus, IModule )

public:
    EventBus()                               = default;
    EventBus( const EventBus& )            = delete;
    EventBus& operator=( const EventBus& ) = delete;

    Error init() override;
    void finalize() override;

    /**
     * @brief Subscribe to an event type
     *
     * The order of subscribed consumers defines the event pipeline.
     * Subscribing to the base Event type is not supported.
     *
     * @tparam T Event type to subscribe to
     * @param callback Function to call when event is published
     * @return Subscription ID (can be used to unsubscribe)
     */
    template<std::derived_from<Event> T>
    size_t subscribe( std::function<void( T& )> callback )
    {
        size_t index = next_subscription_id++;
        auto wrapper = [callback]( Event& event ) { callback( static_cast<T&>( event ) ); };
        subscribers[rfl::Registry::get_type_id<T>()].emplace_back( index, wrapper );
        return index;
    }

    /**
     * @brief Unsubscribe from an event
     * @param p_subscription_id ID returned from subscribe()
     */
    void unsubscribe( size_t p_subscription_id );

    /**
     * @brief Publish an event immediately (synchronous)
     * @tparam T Event type
     * @param event Event instance to publish
     */
    template<std::derived_from<Event> T>
    void publish( const T& event )
    {
        auto it = subscribers.find( event.get_type_id() );
        if (it == subscribers.end())
            return;

        for (auto& [index, callback] : it->second) {
            callback( event );
            if (event.handled)
                break; // Stop propagation if marked read handled
        }
    }

    /**
     * @brief Queue an event for deferred processing
     * @tparam T Event type
     * @param event Event instance to queue
     *
     * Queued get_events are processed when ProcessQueue() is called.
     * Useful for get_events that should not be processed immediately.
     */
    template<std::derived_from<Event> T>
    void enqueue( std::unique_ptr<T> event )
    {
        event_queue.push_back( std::move( event ) );
    }

    void clear();

    /**
     * @brief Process all queued get_events
     *
     * Called once per frame to process get_events that were queued
     * using Queue(). Events are processed in FIFO order.
     */
    void process_queue();

    size_t get_queue_size() const;
    using SubscriberDebugInfo = std::vector<std::pair<rfl::TypeId, size_t>>;
    SubscriberDebugInfo get_subscriber_debug_info() const;

private:
    using CallbackType   = std::function<void( Event& )>;
    using SubscriberList = std::vector<std::pair<size_t, CallbackType>>;
    std::unordered_map<rfl::TypeId, SubscriberList> subscribers;

    std::vector<std::unique_ptr<Event>> event_queue;

    size_t next_subscription_id = 0;
};

} // namespace nc
