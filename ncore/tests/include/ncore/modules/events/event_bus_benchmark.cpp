#include <ncore/modules/events/event_bus.h>
#include <ncore/modules/events/input_event.h>

#include <catch2/benchmark/catch_benchmark_all.hpp>
#include <catch2/catch_test_macros.hpp>

namespace nc {
namespace {

using Catch::Benchmark::Chronometer;

// =========================================================================
// Lightweight benchmark event types (each has unique compile-time TypeId)
// =========================================================================

struct BenchEvent : Event {
    NCLASS( BenchEvent, Event )
    EventType get_type() const override
    {
        return EventType::UNKNOWN;
    }

public:
    int payload = 0;
};

struct BenchEventA : Event {
    NCLASS( BenchEventA, Event )
    EventType get_type() const override
    {
        return EventType::UNKNOWN;
    }
};
struct BenchEventB : Event {
    NCLASS( BenchEventB, Event )
    EventType get_type() const override
    {
        return EventType::UNKNOWN;
    }
};
struct BenchEventC : Event {
    NCLASS( BenchEventC, Event )
    EventType get_type() const override
    {
        return EventType::UNKNOWN;
    }
};
struct BenchEventD : Event {
    NCLASS( BenchEventD, Event )
    EventType get_type() const override
    {
        return EventType::UNKNOWN;
    }
};
struct BenchEventE : Event {
    NCLASS( BenchEventE, Event )
    EventType get_type() const override
    {
        return EventType::UNKNOWN;
    }
};
struct BenchEventF : Event {
    NCLASS( BenchEventF, Event )
    EventType get_type() const override
    {
        return EventType::UNKNOWN;
    }
};
struct BenchEventG : Event {
    NCLASS( BenchEventG, Event )
    EventType get_type() const override
    {
        return EventType::UNKNOWN;
    }
};
struct BenchEventH : Event {
    NCLASS( BenchEventH, Event )
    EventType get_type() const override
    {
        return EventType::UNKNOWN;
    }
};
struct BenchEventI : Event {
    NCLASS( BenchEventI, Event )
    EventType get_type() const override
    {
        return EventType::UNKNOWN;
    }
};
struct BenchEventJ : Event {
    NCLASS( BenchEventJ, Event )
    EventType get_type() const override
    {
        return EventType::UNKNOWN;
    }
};

// =========================================================================
// No-op callbacks
// =========================================================================

static void noop( BenchEvent& ) {}
static void noopA( BenchEventA& ) {}
static void noopB( BenchEventB& ) {}
static void noopC( BenchEventC& ) {}
static void noopD( BenchEventD& ) {}
static void noopE( BenchEventE& ) {}
static void noopF( BenchEventF& ) {}
static void noopG( BenchEventG& ) {}
static void noopH( BenchEventH& ) {}
static void noopI( BenchEventI& ) {}
static void noopJ( BenchEventJ& ) {}

// =========================================================================
// Subscribe — the operation itself is the measurement target
// =========================================================================

TEST_CASE( "EventBus subscribe", "[!benchmark][EventBus][subscribe]" )
{
    BENCHMARK( "subscribe 1" )
    {
        EventBus bus;
        return bus.subscribe<BenchEvent>( noop );
    };

    BENCHMARK( "subscribe 10" )
    {
        EventBus bus;
        for (int i = 0; i < 10; ++i)
            bus.subscribe<BenchEvent>( noop );
        return bus.get_subscriber_debug_info().size();
    };

    BENCHMARK( "subscribe 100" )
    {
        EventBus bus;
        for (int i = 0; i < 100; ++i)
            bus.subscribe<BenchEvent>( noop );
        return bus.get_subscriber_debug_info().size();
    };

    BENCHMARK( "subscribe 10 types x 1 each" )
    {
        EventBus bus;
        bus.subscribe<BenchEventA>( noopA );
        bus.subscribe<BenchEventB>( noopB );
        bus.subscribe<BenchEventC>( noopC );
        bus.subscribe<BenchEventD>( noopD );
        bus.subscribe<BenchEventE>( noopE );
        bus.subscribe<BenchEventF>( noopF );
        bus.subscribe<BenchEventG>( noopG );
        bus.subscribe<BenchEventH>( noopH );
        bus.subscribe<BenchEventI>( noopI );
        bus.subscribe<BenchEventJ>( noopJ );
        return bus.get_subscriber_debug_info().size();
    };
}

// =========================================================================
// Unsubscribe — includes subscribe setup cost (mostly linear scan)
// =========================================================================

TEST_CASE( "EventBus unsubscribe", "[!benchmark][EventBus][unsubscribe]" )
{
    BENCHMARK( "unsubscribe only subscriber" )
    {
        EventBus bus;
        auto id = bus.subscribe<BenchEvent>( noop );
        bus.unsubscribe( id );
        return bus.get_subscriber_debug_info().size();
    };

    BENCHMARK( "unsubscribe from 100 (first)" )
    {
        EventBus bus;
        auto target = bus.subscribe<BenchEvent>( noop );
        for (int i = 0; i < 99; ++i)
            bus.subscribe<BenchEvent>( noop );
        bus.unsubscribe( target );
        return bus.get_subscriber_debug_info().size();
    };

    BENCHMARK( "unsubscribe from 100 (last)" )
    {
        EventBus bus;
        for (int i = 0; i < 99; ++i)
            bus.subscribe<BenchEvent>( noop );
        auto target = bus.subscribe<BenchEvent>( noop );
        bus.unsubscribe( target );
        return bus.get_subscriber_debug_info().size();
    };

    BENCHMARK( "unsubscribe missing (full scan 100)" )
    {
        EventBus bus;
        for (int i = 0; i < 100; ++i)
            bus.subscribe<BenchEvent>( noop );
        bus.unsubscribe( 999 );
        return bus.get_subscriber_debug_info().size();
    };
}

// =========================================================================
// Publish — BENCHMARK_ADVANCED separates setup (subscribe) from measurement
// =========================================================================

TEST_CASE( "EventBus publish", "[!benchmark][EventBus][publish]" )
{
    BENCHMARK_ADVANCED( "publish 0 subscribers" )
    ( Chronometer meter )
    {
        EventBus bus;
        meter.measure( [&] {
            auto e = Ref<BenchEvent>::create();
            bus.publish( e );
        } );
    };

    BENCHMARK_ADVANCED( "publish 1 subscriber" )
    ( Chronometer meter )
    {
        EventBus bus;
        bus.subscribe<BenchEvent>( noop );
        meter.measure( [&] {
            auto e = Ref<BenchEvent>::create();
            bus.publish( e );
        } );
    };

    BENCHMARK_ADVANCED( "publish 10 subscribers" )
    ( Chronometer meter )
    {
        EventBus bus;
        for (int i = 0; i < 10; ++i)
            bus.subscribe<BenchEvent>( noop );
        meter.measure( [&] {
            auto e = Ref<BenchEvent>::create();
            bus.publish( e );
        } );
    };

    BENCHMARK_ADVANCED( "publish 100 subscribers" )
    ( Chronometer meter )
    {
        EventBus bus;
        for (int i = 0; i < 100; ++i)
            bus.subscribe<BenchEvent>( noop );
        meter.measure( [&] {
            auto e = Ref<BenchEvent>::create();
            bus.publish( e );
        } );
    };

    BENCHMARK_ADVANCED( "publish 100 subscribers (early exit 1st)" )
    ( Chronometer meter )
    {
        EventBus bus;
        bus.subscribe<BenchEvent>( []( BenchEvent& e ) { e.handled = true; } );
        for (int i = 0; i < 99; ++i)
            bus.subscribe<BenchEvent>( noop );
        meter.measure( [&] {
            auto e = Ref<BenchEvent>::create();
            bus.publish( e );
        } );
    };

    BENCHMARK_ADVANCED( "publish 100 subscribers (early exit 50th)" )
    ( Chronometer meter )
    {
        EventBus bus;
        for (int i = 0; i < 49; ++i)
            bus.subscribe<BenchEvent>( noop );
        bus.subscribe<BenchEvent>( []( BenchEvent& e ) { e.handled = true; } );
        for (int i = 0; i < 50; ++i)
            bus.subscribe<BenchEvent>( noop );
        meter.measure( [&] {
            auto e = Ref<BenchEvent>::create();
            bus.publish( e );
        } );
    };

    BENCHMARK_ADVANCED( "publish with 10 other types registered" )
    ( Chronometer meter )
    {
        EventBus bus;
        bus.subscribe<BenchEventA>( noopA );
        bus.subscribe<BenchEventB>( noopB );
        bus.subscribe<BenchEventC>( noopC );
        bus.subscribe<BenchEventD>( noopD );
        bus.subscribe<BenchEventE>( noopE );
        bus.subscribe<BenchEventF>( noopF );
        bus.subscribe<BenchEventG>( noopG );
        bus.subscribe<BenchEventH>( noopH );
        bus.subscribe<BenchEventI>( noopI );
        bus.subscribe<BenchEventJ>( noopJ );
        bus.subscribe<BenchEvent>( noop );
        meter.measure( [&] {
            auto e = Ref<BenchEvent>::create();
            bus.publish( e );
        } );
    };
}

// =========================================================================
// Enqueue — includes make_unique cost (realistic for the engine's usage)
// =========================================================================

TEST_CASE( "EventBus enqueue", "[!benchmark][EventBus][enqueue]" )
{
    BENCHMARK( "enqueue 1 event" )
    {
        EventBus bus;
        bus.enqueue( Ref<BenchEvent>::create() );
        return bus.get_queue_size();
    };

    BENCHMARK( "enqueue 10 events" )
    {
        EventBus bus;
        for (int i = 0; i < 10; ++i)
            bus.enqueue( Ref<BenchEvent>::create() );
        return bus.get_queue_size();
    };

    BENCHMARK( "enqueue 100 events" )
    {
        EventBus bus;
        for (int i = 0; i < 100; ++i)
            bus.enqueue( Ref<BenchEvent>::create() );
        return bus.get_queue_size();
    };

    BENCHMARK( "enqueue 1000 events" )
    {
        EventBus bus;
        for (int i = 0; i < 1000; ++i)
            bus.enqueue( Ref<MouseMotionEvent>::create( 0, Vec2{}, Vec2{}, 0 ) );
        return bus.get_queue_size();
    };
}

// =========================================================================
// Process queue — BENCHMARK_ADVANCED separates subscription setup from
// the measured enqueue+dispatch cycle (which clears the queue each time)
// =========================================================================

TEST_CASE( "EventBus process_queue", "[!benchmark][EventBus][process_queue]" )
{
    BENCHMARK_ADVANCED( "process empty queue" )
    ( Chronometer meter )
    {
        EventBus bus;
        meter.measure( [&] { bus.flush(); } );
    };

    BENCHMARK_ADVANCED( "process 10 events x 1 subscriber" )
    ( Chronometer meter )
    {
        EventBus bus;
        bus.subscribe<BenchEvent>( noop );
        meter.measure( [&] {
            for (int i = 0; i < 10; ++i)
                bus.enqueue( Ref<BenchEvent>::create() );
            bus.flush();
        } );
    };

    BENCHMARK_ADVANCED( "process 100 events x 1 subscriber" )
    ( Chronometer meter )
    {
        EventBus bus;
        bus.subscribe<BenchEvent>( noop );
        meter.measure( [&] {
            for (int i = 0; i < 100; ++i)
                bus.enqueue( Ref<BenchEvent>::create() );
            bus.flush();
        } );
    };

    BENCHMARK_ADVANCED( "process 100 events x 0 subscribers (no match)" )
    ( Chronometer meter )
    {
        EventBus bus;
        meter.measure( [&] {
            for (int i = 0; i < 100; ++i)
                bus.enqueue( Ref<BenchEvent>::create() );
            bus.flush();
        } );
    };

    BENCHMARK_ADVANCED( "process 100 events x 10 subscribers each" )
    ( Chronometer meter )
    {
        EventBus bus;
        for (int i = 0; i < 10; ++i)
            bus.subscribe<BenchEvent>( noop );
        meter.measure( [&] {
            for (int i = 0; i < 100; ++i)
                bus.enqueue( Ref<BenchEvent>::create() );
            bus.flush();
        } );
    };

    BENCHMARK_ADVANCED( "process 50 events mixed 3 types" )
    ( Chronometer meter )
    {
        EventBus bus;
        bus.subscribe<BenchEventA>( noopA );
        bus.subscribe<BenchEventB>( noopB );
        bus.subscribe<BenchEventC>( noopC );
        meter.measure( [&] {
            for (int i = 0; i < 17; ++i)
                bus.enqueue( Ref<BenchEventA>::create() );
            for (int i = 0; i < 16; ++i)
                bus.enqueue( Ref<BenchEventB>::create() );
            for (int i = 0; i < 17; ++i)
                bus.enqueue( Ref<BenchEventC>::create() );
            bus.flush();
        } );
    };
}

// =========================================================================
// Frame simulation — realistic per-frame workload
// =========================================================================

TEST_CASE( "EventBus frame simulation", "[!benchmark][EventBus][frame]" )
{
    BENCHMARK_ADVANCED( "typical: 200 mouse events + 5 subscribers" )
    ( Chronometer meter )
    {
        EventBus bus;
        bus.subscribe<WindowCloseEvent>( []( WindowCloseEvent& ) {} );
        bus.subscribe<WindowResizeEvent>( []( WindowResizeEvent& ) {} );
        bus.subscribe<MouseMotionEvent>( []( MouseMotionEvent& ) {} );
        bus.subscribe<KeyboardEvent>( []( KeyboardEvent& ) {} );
        bus.subscribe<MouseButtonEvent>( []( MouseButtonEvent& ) {} );

        meter.measure( [&] {
            for (int i = 0; i < 200; ++i)
                bus.enqueue( Ref<MouseMotionEvent>::create( 0, Vec2{}, Vec2{}, 0 ) );
            bus.flush();
        } );
    };

    BENCHMARK_ADVANCED( "light: 10 events + 3 subscribers" )
    ( Chronometer meter )
    {
        EventBus bus;
        bus.subscribe<WindowCloseEvent>( []( WindowCloseEvent& ) {} );
        bus.subscribe<WindowResizeEvent>( []( WindowResizeEvent& ) {} );
        bus.subscribe<MouseMotionEvent>( []( MouseMotionEvent& ) {} );

        meter.measure( [&] {
            bus.enqueue( Ref<MouseMotionEvent>::create( 0, Vec2{}, Vec2{}, 0 ) );
            bus.enqueue( Ref<MouseMotionEvent>::create( 0, Vec2{}, Vec2{}, 0 ) );
            bus.enqueue( Ref<MouseMotionEvent>::create( 0, Vec2{}, Vec2{}, 0 ) );
            bus.enqueue( Ref<KeyboardEvent>::create( 0, ButtonAction::PRESS, KeyboardEvent::Key::W, false ) );
            bus.enqueue( Ref<KeyboardEvent>::create( 0, ButtonAction::RELEASE, KeyboardEvent::Key::W, false ) );
            bus.enqueue( Ref<MouseMotionEvent>::create( 0, Vec2{}, Vec2{}, 0 ) );
            bus.enqueue( Ref<MouseMotionEvent>::create( 0, Vec2{}, Vec2{}, 0 ) );
            bus.enqueue( Ref<MouseMotionEvent>::create( 0, Vec2{}, Vec2{}, 0 ) );
            bus.enqueue( Ref<MouseMotionEvent>::create( 0, Vec2{}, Vec2{}, 0 ) );
            bus.enqueue( Ref<MouseMotionEvent>::create( 0, Vec2{}, Vec2{}, 0 ) );
            bus.flush();
        } );
    };
}

// =========================================================================
// Callback overhead — free function vs capturing lambda
// =========================================================================

TEST_CASE( "EventBus callback overhead", "[!benchmark][EventBus][callback]" )
{
    volatile int sink = 0;

    BENCHMARK_ADVANCED( "free function callback" )
    ( Chronometer meter )
    {
        EventBus bus;
        bus.subscribe<BenchEvent>( [&sink]( BenchEvent& e ) { sink = e.payload; } );
        meter.measure( [&] {
            auto e = Ref<BenchEvent>::create();
            bus.publish( e );
        } );
    };

    int capture_me = 42;
    BENCHMARK_ADVANCED( "capturing lambda callback" )
    ( Chronometer meter )
    {
        EventBus bus;
        bus.subscribe<BenchEvent>( [&sink, capture_me]( BenchEvent& e ) {
            sink = e.payload + capture_me;
        } );
        meter.measure( [&] {
            auto e = Ref<BenchEvent>::create();
            bus.publish( e );
        } );
    };

    BENCHMARK_ADVANCED( "10 free function callbacks" )
    ( Chronometer meter )
    {
        EventBus bus;
        for (int i = 0; i < 10; ++i)
            bus.subscribe<BenchEvent>( [&sink]( BenchEvent& e ) { sink = e.payload; } );
        meter.measure( [&] {
            auto e = Ref<BenchEvent>::create();
            bus.publish( e );
        } );
    };
}

} // anonymous namespace
} // namespace nc
