#pragma once

#include <functional>

#include <ncore/kernel/collection.h>
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
    EventBus()                             = default;
    EventBus( const EventBus& )            = delete;
    EventBus& operator=( const EventBus& ) = delete;

    Error init() override;
    void finalize() override;

    /**
     * @brief Subscribe to an event type
     *
     * The order of subscribed consumers defines the event pipeline.
     * Subscribing to BaseEvent type is not supported.
     *
     * @tparam T Event type to subscribe to
     * @param callback Function to call when event is published
     * @return Subscription ID (can be used to unsubscribe)
     */
    template<std::derived_from<BaseEvent> T>
    size_t subscribe( std::function<void( T& )> callback )
    {
        size_t index = next_subscription_id++;
        auto wrapper = [callback]( BaseEvent& event ) { callback( static_cast<T&>( event ) ); };
        subscribers[rtti::Registry::get_type_id<T>()].emplace_back( index, wrapper );
        return index;
    }

    /**
     * @brief Unsubscribe from an event
     * @param p_subscription_id ID returned from subscribe()
     */
    void unsubscribe( size_t p_subscription_id );

    /**
     * @brief Publish an event immediately (synchronous)
     * @param event Event instance to publish
     */
    void publish( const Ref<BaseEvent>& event )
    {
        auto it = subscribers.find( event->get_type_id() );
        if (it == subscribers.end())
            return;

        for (auto& [index, callback] : it->second) {
            callback( const_cast<BaseEvent&>( *event ) );
            if (event->handled)
                break; // Stop propagation if marked read handled
        }
    }

    /**
     * @brief Queue an event for deferred processing
     * @param event Event instance to queue
     *
     * Queued events are processed when ProcessQueue() is called.
     * Useful for events that should not be processed immediately.
     */
    void enqueue( const Ref<BaseEvent>& event )
    {
        event_queue.push_back( event );
    }

    void clear();

    /**
     * @brief Process all queued events
     *
     * Called once per frame to process events that were queued
     * using Queue(). Events are processed in FIFO order.
     */
    void flush();

    size_t get_queue_size() const;
    using SubscriberDebugInfo = std::vector<std::pair<rtti::TypeId, size_t>>;
    SubscriberDebugInfo get_subscriber_debug_info() const;

private:
    using CallbackType   = std::function<void( BaseEvent& )>;
    using SubscriberList = std::vector<std::pair<size_t, CallbackType>>;
    std::unordered_map<rtti::TypeId, SubscriberList> subscribers;

    Vector<Ref<BaseEvent>> event_queue;

    size_t next_subscription_id = 0;
};

} // namespace nc
