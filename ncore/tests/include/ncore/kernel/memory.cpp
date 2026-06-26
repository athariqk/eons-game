#include <cstdint>
#include <iostream>
#include <vector>

#include <ncore/kernel/memory.h>

#include <catch2/benchmark/catch_benchmark.hpp>
#include <catch2/catch_test_macros.hpp>

// ---------------------------------------------------------------------------
// Helpers
// ---------------------------------------------------------------------------

// Convenience: allocate raw slot and placement-new in one call.
// Mirrors what a real manager (ObjectPool, ECS) would do on top of the arena.
template<typename T, typename... Args>
T* arena_new( ncore::PagedAllocator<T>& arena, Args&&... args )
{
    T* slot = arena.alloc();
    return new ( slot ) T( std::forward<Args>( args )... );
}

// Mirrors what a real manager would do when releasing an object.
template<typename T>
void arena_delete( T* ptr )
{
    ptr->~T();
}

// ---------------------------------------------------------------------------
// Test fixtures
// ---------------------------------------------------------------------------

struct Entity {
    size_t id;
    size_t expected_page;
    uint64_t magic_canary;

    Entity( size_t id, size_t page ) : id( id ), expected_page( page ), magic_canary( id ^ 0xDEAD'BEEF'DEAD'BEEFull ) {}
};

// ---------------------------------------------------------------------------
// Pointer-stability suite
// ---------------------------------------------------------------------------

TEST_CASE( "PagedAllocator guarantees 100% pointer stability", "[PagedAllocator][Memory]" )
{

    constexpr size_t PAGE_SIZE      = 4;
    constexpr size_t TOTAL_ENTITIES = 50'000;
    static_assert(
        TOTAL_ENTITIES % PAGE_SIZE == 0,
        "TOTAL_ENTITIES must be a multiple of PAGE_SIZE for exact page-count assertions."
    );

    ncore::PagedAllocator<Entity> arena( PAGE_SIZE );

    std::vector<Entity*> tracker;
    tracker.reserve( TOTAL_ENTITIES );

    SECTION( "alloc() never returns null" )
    {
        for (size_t i = 0; i < TOTAL_ENTITIES; ++i) {
            Entity* ptr = arena.alloc();
            CAPTURE( i );
            REQUIRE( ptr != nullptr );
            tracker.push_back( ptr );
        }
    }

    SECTION( "Pointers remain valid after massive page growths" )
    {
        for (size_t i = 0; i < TOTAL_ENTITIES; ++i) {
            // Raw alloc + manual construct — arena_new() shows what a manager does
            Entity* ptr = arena_new( arena, i, i / PAGE_SIZE );
            REQUIRE( ptr != nullptr );
            tracker.push_back( ptr );
        }

        REQUIRE( arena.get_page_count() == TOTAL_ENTITIES / PAGE_SIZE );
        REQUIRE( arena.get_size() == TOTAL_ENTITIES );
        REQUIRE( arena.get_page_capacity() == PAGE_SIZE );

        size_t corrupted_count = 0;
        size_t null_count      = 0;

        for (size_t i = 0; i < TOTAL_ENTITIES; ++i) {
            Entity* ptr = tracker[i];

            if (ptr == nullptr) {
                ++null_count;
                continue;
            }

            bool is_valid = ( ptr->id == i ) && ( ptr->expected_page == i / PAGE_SIZE ) &&
                            ( ptr->magic_canary == ( i ^ 0xDEAD'BEEF'DEAD'BEEFull ) );

            if (!is_valid) {
                CAPTURE( i, ptr->id, ptr->expected_page, ptr->magic_canary );
                ++corrupted_count;
            }
        }

        CHECK( null_count == 0 );
        CHECK( corrupted_count == 0 );

        if (null_count || corrupted_count) {
            FAIL( "Detected " << null_count << " null pointer(s) and " << corrupted_count << " corrupted entity/ies." );
        }

        // Manual cleanup — the arena owns raw memory, not object lifetimes
        for (Entity* ptr : tracker)
            arena_delete( ptr );
    }
}

// ---------------------------------------------------------------------------
// Lifecycle suite
//
// PagedAllocator is lifetime-agnostic: it hands out raw slots and frees raw
// pages. Construction and destruction are the caller's responsibility.
// These tests verify that the arena's memory is safe to construct into and
// that the caller controls exactly when destructors fire.
// ---------------------------------------------------------------------------

namespace {
struct LiveCount {
    static int& value()
    {
        static int n = 0;
        return n;
    }
};

struct LifecycleTracker {
    LifecycleTracker()
    {
        ++LiveCount::value();
    }
    ~LifecycleTracker()
    {
        --LiveCount::value();
    }

    LifecycleTracker( const LifecycleTracker& )            = delete;
    LifecycleTracker& operator=( const LifecycleTracker& ) = delete;
};
} // anonymous namespace

TEST_CASE( "PagedAllocator caller controls object lifecycles", "[PagedAllocator][Lifecycle]" )
{
    LiveCount::value() = 0;

    SECTION( "Constructor fires only when caller placement-news into the slot" )
    {
        ncore::PagedAllocator<LifecycleTracker> arena( 10 );

        // Raw alloc — no construction yet
        LifecycleTracker* slot = arena.alloc();
        REQUIRE( LiveCount::value() == 0 );

        // Explicit construct
        new ( slot ) LifecycleTracker();
        REQUIRE( LiveCount::value() == 1 );

        // Explicit destroy
        arena_delete( slot );
        REQUIRE( LiveCount::value() == 0 );
    }

    SECTION( "Arena destruction does NOT call destructors — caller must" )
    {
        {
            ncore::PagedAllocator<LifecycleTracker> arena( 10 );
            std::vector<LifecycleTracker*> live;

            for (int i = 0; i < 25; ++i)
                live.push_back( arena_new( arena /* no args — default ctor */ ) );

            REQUIRE( LiveCount::value() == 25 );

            // Caller destroys objects before arena goes out of scope
            for (auto* p : live)
                arena_delete( p );

            REQUIRE( LiveCount::value() == 0 );
        } // arena freed here — raw pages deleted, no destructors called (already done)

        REQUIRE( LiveCount::value() == 0 );
    }

    SECTION( "Skipping destructor before arena dealloc is a detectable leak" )
    {
        // This section documents the contract violation: if the caller forgets
        // to call destructors, LiveCount stays non-zero after the arena is gone.
        // In production this would be a resource leak.
        {
            ncore::PagedAllocator<LifecycleTracker> arena( 10 );
            for (int i = 0; i < 5; ++i)
                arena_new( arena );

            REQUIRE( LiveCount::value() == 5 );

            // Deliberately NOT calling arena_delete — simulates a caller bug.
            // The arena frees raw memory; destructors never fire.
        }

        // Proves the arena did NOT silently clean up: LiveCount is still 5.
        // A well-behaved caller would have driven this to 0 before dealloc.
        REQUIRE( LiveCount::value() == 5 );

        // Clean up so subsequent tests start fresh
        LiveCount::value() = 0;
    }

    SECTION( "Incomplete final page: destructor count is exact" )
    {
        ncore::PagedAllocator<LifecycleTracker> arena( 10 );
        std::vector<LifecycleTracker*> live;

        // 23 objects across 3 pages (10 + 10 + 3)
        for (int i = 0; i < 23; ++i)
            live.push_back( arena_new( arena ) );

        REQUIRE( LiveCount::value() == 23 );

        for (auto* p : live)
            arena_delete( p );

        REQUIRE( LiveCount::value() == 0 );
    }
}

// ---------------------------------------------------------------------------
// Alignment suite
// ---------------------------------------------------------------------------

struct alignas( 32 ) HeavyMathVector {
    float data[8];
    HeavyMathVector()
    {
        for (float& f : data)
            f = 1.0f;
    }
};

TEST_CASE( "PagedAllocator respects hardware memory alignment", "[PagedAllocator][Alignment]" )
{

    SECTION( "Returned pointers are correctly aligned for SIMD operations" )
    {
        ncore::PagedAllocator<HeavyMathVector> arena( 8 );

        for (int i = 0; i < 20; ++i) {
            HeavyMathVector* ptr = arena.alloc();
            REQUIRE( ptr != nullptr );

            std::uintptr_t addr = reinterpret_cast<std::uintptr_t>( ptr );
            CAPTURE( i, addr );
            REQUIRE( addr % alignof( HeavyMathVector ) == 0 );

            // Construct and do a write/read to confirm the slot is actually usable
            new ( ptr ) HeavyMathVector();
            ptr->data[0] = static_cast<float>( i );
            REQUIRE( ptr->data[0] == static_cast<float>( i ) );
            ptr->~HeavyMathVector();
        }
    }

    SECTION( "Alignment is preserved across page boundaries" )
    {
        // Page size 3 (rounds up to 4 via bit_ceil): crosses pages frequently
        ncore::PagedAllocator<HeavyMathVector> arena( 3 );

        for (int i = 0; i < 12; ++i) {
            HeavyMathVector* ptr = arena.alloc();
            REQUIRE( ptr != nullptr );
            std::uintptr_t addr = reinterpret_cast<std::uintptr_t>( ptr );
            CAPTURE( i, addr );
            REQUIRE( addr % alignof( HeavyMathVector ) == 0 );
        }
    }
}

// ---------------------------------------------------------------------------
// Edge-case suite
// ---------------------------------------------------------------------------

TEST_CASE( "PagedAllocator handles extreme edge cases", "[PagedAllocator][EdgeCases]" )
{

    SECTION( "Page size of 1 allocates a new page per object" )
    {
        ncore::PagedAllocator<int> arena( 1 );

        int* a = arena.alloc();
        new ( a ) int( 10 );
        int* b = arena.alloc();
        new ( b ) int( 20 );
        int* c = arena.alloc();
        new ( c ) int( 30 );

        REQUIRE( a != nullptr );
        REQUIRE( b != nullptr );
        REQUIRE( c != nullptr );

        REQUIRE( *a == 10 );
        REQUIRE( *b == 20 );
        REQUIRE( *c == 30 );

        REQUIRE( a != b );
        REQUIRE( b != c );
        REQUIRE( a != c );

        REQUIRE( arena.get_page_count() == 3 );
        REQUIRE( arena.get_size() == 3 );
    }

    SECTION( "Single allocation fills and saturates a one-element page" )
    {
        ncore::PagedAllocator<int> arena( 1 );
        int* only = arena.alloc();
        new ( only ) int( 42 );

        REQUIRE( only != nullptr );
        REQUIRE( *only == 42 );
        REQUIRE( arena.get_page_count() == 1 );
        REQUIRE( arena.get_size() == 1 );
    }

    SECTION( "reset() clears size but pages remain allocated and reusable" )
    {
        ncore::PagedAllocator<int> arena( 4 );

        int* first = arena.alloc();
        new ( first ) int( 99 );
        REQUIRE( arena.get_size() == 1 );
        REQUIRE( arena.get_page_count() == 1 );

        arena.reset();
        REQUIRE( arena.get_size() == 0 );
        REQUIRE( arena.get_page_count() == 1 ); // page still alive

        // The same page slot is now reusable
        int* reused = arena.alloc();
        REQUIRE( reused != nullptr );
        new ( reused ) int( 77 );
        REQUIRE( *reused == 77 );
    }
}

// ---------------------------------------------------------------------------
// Memory-footprint suite
// ---------------------------------------------------------------------------

template<typename T>
size_t get_total_memory_footprint( ncore::PagedAllocator<T>& arena )
{
    size_t page_memory   = arena.get_page_count() * arena.get_page_capacity() * sizeof( T );
    size_t vector_memory = arena.get_page_count() * sizeof( T* );
    size_t base_memory   = sizeof( arena );
    return page_memory + vector_memory + base_memory;
}

template<typename T>
size_t get_wasted_memory( ncore::PagedAllocator<T>& arena )
{
    return get_total_memory_footprint( arena ) - arena.get_size() * sizeof( T );
}

struct alignas( 16 ) Transform {
    float matrix[16];
    Transform()
    {
        for (float& f : matrix)
            f = 1.0f;
    }
};

template<class T>
struct TrackingAllocator {
    using value_type = T;

    size_t* live_bytes;
    size_t* allocation_count;

    explicit TrackingAllocator( size_t& lb, size_t& ac ) : live_bytes( &lb ), allocation_count( &ac ) {}

    template<class U>
    TrackingAllocator( const TrackingAllocator<U>& o ) noexcept :
        live_bytes( o.live_bytes ), allocation_count( o.allocation_count )
    {}

    T* allocate( std::size_t n )
    {
        *live_bytes += n * sizeof( T ) + 16;
        *allocation_count += 1;
        return static_cast<T*>( ::operator new( n * sizeof( T ) ) );
    }

    void deallocate( T* p, std::size_t n ) noexcept
    {
        *live_bytes -= n * sizeof( T ) + 16;
        ::operator delete( p );
    }
};

TEST_CASE( "Memory footprint PagedAllocator vs std vector", "[Memory][Footprint]" )
{

    constexpr size_t ALLOC_COUNT = 4'100;

    SECTION( "PagedAllocator wastes less RAM than an unreserved std vector" )
    {
        size_t vec_live_bytes  = 0;
        size_t vec_alloc_count = 0;

        using TrackedVec = std::vector<Transform, TrackingAllocator<Transform>>;
        TrackedVec vec{ TrackingAllocator<Transform>{ vec_live_bytes, vec_alloc_count } };

        for (size_t i = 0; i < ALLOC_COUNT; ++i)
            vec.emplace_back();

        size_t vec_total_ram  = vec_live_bytes + sizeof( vec );
        size_t vec_useful_ram = ALLOC_COUNT * sizeof( Transform );

        REQUIRE( vec_total_ram >= vec_useful_ram );
        size_t vec_wasted = vec_total_ram - vec_useful_ram;

        ncore::PagedAllocator<Transform> arena( 1024 );
        for (size_t i = 0; i < ALLOC_COUNT; ++i)
            arena.alloc();

        size_t arena_wasted = get_wasted_memory( arena );

        INFO( "Vector reallocations : " << vec_alloc_count );
        INFO( "Vector wasted RAM    : " << vec_wasted << " bytes" );
        INFO( "Arena wasted RAM     : " << arena_wasted << " bytes" );

        REQUIRE( arena_wasted < vec_wasted );
    }

    SECTION( "PagedAllocator footprint scales linearly with page count" )
    {
        constexpr size_t PAGE_CAP = 128;
        ncore::PagedAllocator<Transform> arena( PAGE_CAP );

        for (size_t i = 0; i < PAGE_CAP * 4; ++i)
            arena.alloc();

        size_t expected_page_ram = arena.get_page_count() * PAGE_CAP * sizeof( Transform );
        size_t actual_page_ram   = arena.get_page_count() * arena.get_page_capacity() * sizeof( Transform );

        REQUIRE( actual_page_ram == expected_page_ram );
        REQUIRE( arena.get_page_count() == 4 );
    }
}

// ---------------------------------------------------------------------------
// Benchmarks
// ---------------------------------------------------------------------------

TEST_CASE( "Benchmark PagedAllocator vs std vector", "[PagedAllocator][Benchmark]" )
{
    constexpr size_t ALLOC_COUNT = 1'000'000;

    BENCHMARK( "std vector (no reserve)" )
    {
        std::vector<Transform> vec;
        for (size_t i = 0; i < ALLOC_COUNT; ++i)
            vec.emplace_back();
        return vec.size();
    };

    BENCHMARK( "std vector (pre-reserved)" )
    {
        std::vector<Transform> vec;
        vec.reserve( ALLOC_COUNT );
        for (size_t i = 0; i < ALLOC_COUNT; ++i)
            vec.emplace_back();
        return vec.size();
    };

    BENCHMARK( "PagedAllocator (page size 4096)" )
    {
        ncore::PagedAllocator<Transform> arena( 4096 );
        for (size_t i = 0; i < ALLOC_COUNT; ++i)
            arena.alloc();
        return arena.get_size();
    };
}
