#include <ncore/kernel/collection.h>

#include <catch2/catch_test_macros.hpp>

// ---------------------------------------------------------------------------
// Test fixtures
// ---------------------------------------------------------------------------

namespace {

struct LiveCount {
    static int& value()
    {
        static int n = 0;
        return n;
    }
};

struct Tracked {
    int value;
    explicit Tracked(int v) : value(v)
    {
        ++LiveCount::value();
    }
    ~Tracked()
    {
        --LiveCount::value();
    }

    Tracked(const Tracked&)            = delete;
    Tracked& operator=(const Tracked&) = delete;
};

struct alignas(32) SIMDVec {
    float data[8];
    explicit SIMDVec(float v = 0.f)
    {
        for (float& f : data)
            f = v;
    }
};

} // anonymous namespace

// ---------------------------------------------------------------------------
// Acquire / release basics
// ---------------------------------------------------------------------------

TEST_CASE("PagedObjectPool acquire and release basics", "[PagedObjectPool][Acquire]")
{

    SECTION("Acquired pointer is non-null and holds constructed value")
    {
        ncore::PagedObjectPool<int> pool(4);
        int* a = pool.acquire(42);
        REQUIRE(a != nullptr);
        REQUIRE(*a == 42);
        pool.release(a);
    }

    SECTION("active_count tracks live objects exactly")
    {
        ncore::PagedObjectPool<int> pool(4);
        int* a = pool.acquire(1);
        int* b = pool.acquire(2);
        int* c = pool.acquire(3);
        REQUIRE(pool.get_active_count() == 3);

        pool.release(b);
        REQUIRE(pool.get_active_count() == 2);

        pool.release(a);
        pool.release(c);
        REQUIRE(pool.get_active_count() == 0);
    }

    SECTION("All acquired pointers are distinct")
    {
        ncore::PagedObjectPool<int> pool(4);
        int* a = pool.acquire(1);
        int* b = pool.acquire(2);
        int* c = pool.acquire(3);
        int* d = pool.acquire(4);

        REQUIRE(a != b);
        REQUIRE(a != c);
        REQUIRE(a != d);
        REQUIRE(b != c);
        REQUIRE(b != d);
        REQUIRE(c != d);

        pool.release(a);
        pool.release(b);
        pool.release(c);
        pool.release(d);
    }
}

// ---------------------------------------------------------------------------
// Free list / slot reuse
// ---------------------------------------------------------------------------

TEST_CASE("PagedObjectPool reuses released slots", "[PagedObjectPool][FreeList]")
{

    SECTION("Released slots are reused in LIFO order")
    {
        ncore::PagedObjectPool<int> pool(4);
        int* a = pool.acquire(1);
        int* b = pool.acquire(2);
        int* c = pool.acquire(3);
        int* d = pool.acquire(4);

        pool.release(b);
        pool.release(d);

        // Free list is LIFO: d was released last, so it's reused first
        int* e = pool.acquire(5);
        int* f = pool.acquire(6);

        REQUIRE(e == d);
        REQUIRE(f == b);
        REQUIRE(*e == 5);
        REQUIRE(*f == 6);
        REQUIRE(pool.get_active_count() == 4);

        pool.release(a);
        pool.release(c);
        pool.release(e);
        pool.release(f);
        REQUIRE(pool.get_active_count() == 0);
    }

    SECTION("Reused slot holds newly constructed value, not old one")
    {
        ncore::PagedObjectPool<int> pool(2);
        int* a = pool.acquire(99);
        pool.release(a);

        int* b = pool.acquire(77); // reuses a's slot
        REQUIRE(b == a);           // same address
        REQUIRE(*b == 77);         // new value, not 99
        pool.release(b);
    }

    SECTION("Free list survives multiple acquire/release cycles")
    {
        ncore::PagedObjectPool<int> pool(4);
        std::vector<int*> ptrs;

        // First wave
        for (int i = 0; i < 4; ++i)
            ptrs.push_back(pool.acquire(i));
        for (int* p : ptrs)
            pool.release(p);
        REQUIRE(pool.get_active_count() == 0);

        // Second wave — all slots come from free list, no new pages
        size_t page_count_before = pool.get_page_count();
        for (int i = 0; i < 4; ++i)
            ptrs[i] = pool.acquire(i + 10);
        REQUIRE(pool.get_page_count() == page_count_before);
        REQUIRE(pool.get_active_count() == 4);

        for (int* p : ptrs)
            pool.release(p);
    }
}

// ---------------------------------------------------------------------------
// Page growth
// ---------------------------------------------------------------------------

TEST_CASE("PagedObjectPool grows pages on demand", "[PagedObjectPool][Growth]")
{

    SECTION("Exceeding page capacity allocates a new page")
    {
        ncore::PagedObjectPool<int> pool(2);
        int* a = pool.acquire(1);
        int* b = pool.acquire(2);
        REQUIRE(pool.get_page_count() == 1);

        int* c = pool.acquire(3); // triggers new page
        REQUIRE(pool.get_page_count() == 2);
        REQUIRE(c != nullptr);
        REQUIRE(*c == 3);

        pool.release(a);
        pool.release(b);
        pool.release(c);
        REQUIRE(pool.get_active_count() == 0);
    }

    SECTION("Pointers remain stable after page growth")
    {
        ncore::PagedObjectPool<int> pool(2);
        int* a = pool.acquire(1);
        int* b = pool.acquire(2);

        // Force page growth
        int* c = pool.acquire(3);
        int* d = pool.acquire(4);

        // Original pointers must still be valid
        REQUIRE(*a == 1);
        REQUIRE(*b == 2);

        pool.release(a);
        pool.release(b);
        pool.release(c);
        pool.release(d);
    }

    SECTION("Free list slots are preferred over new page allocation")
    {
        ncore::PagedObjectPool<int> pool(2);
        int* a = pool.acquire(1);
        int* b = pool.acquire(2);
        pool.release(a);
        pool.release(b);

        size_t pages_before = pool.get_page_count();

        // Should reuse free list, not allocate new pages
        int* c = pool.acquire(3);
        int* d = pool.acquire(4);
        REQUIRE(pool.get_page_count() == pages_before);

        pool.release(c);
        pool.release(d);
    }
}

// ---------------------------------------------------------------------------
// Object lifetime / constructor-destructor
// ---------------------------------------------------------------------------

TEST_CASE("PagedObjectPool manages object lifetimes correctly", "[PagedObjectPool][Lifetime]")
{
    LiveCount::value() = 0;

    SECTION("Constructor is called on acquire")
    {
        ncore::PagedObjectPool<Tracked> pool(4);
        Tracked* t = pool.acquire(1);
        REQUIRE(LiveCount::value() == 1);
        REQUIRE(t->value == 1);
        pool.release(t);
    }

    SECTION("Destructor is called on release")
    {
        ncore::PagedObjectPool<Tracked> pool(4);
        Tracked* t = pool.acquire(1);
        REQUIRE(LiveCount::value() == 1);
        pool.release(t);
        REQUIRE(LiveCount::value() == 0);
    }

    SECTION("Constructor and destructor fire exactly once per acquire/release")
    {
        ncore::PagedObjectPool<Tracked> pool(4);
        std::vector<Tracked*> live;

        for (int i = 0; i < 8; ++i)
            live.push_back(pool.acquire(i));

        REQUIRE(LiveCount::value() == 8);

        for (Tracked* t : live)
            pool.release(t);

        REQUIRE(LiveCount::value() == 0);
    }

    SECTION("Reused slot calls destructor then constructor in correct order")
    {
        ncore::PagedObjectPool<Tracked> pool(2);
        Tracked* a = pool.acquire(10);
        pool.release(a); // destructor fires: LiveCount = 0
        REQUIRE(LiveCount::value() == 0);

        Tracked* b = pool.acquire(20); // constructor fires: LiveCount = 1
        REQUIRE(LiveCount::value() == 1);
        REQUIRE(b->value == 20);
        pool.release(b);
        REQUIRE(LiveCount::value() == 0);
    }
}

// ---------------------------------------------------------------------------
// Alignment
// ---------------------------------------------------------------------------

TEST_CASE("PagedObjectPool preserves alignment for over-aligned types", "[PagedObjectPool][Alignment]")
{

    SECTION("Acquired pointers satisfy alignof requirement")
    {
        ncore::PagedObjectPool<SIMDVec> pool(8);

        for (int i = 0; i < 20; ++i) {
            SIMDVec* ptr = pool.acquire(static_cast<float>(i));
            REQUIRE(ptr != nullptr);
            std::uintptr_t addr = reinterpret_cast<std::uintptr_t>(ptr);
            CAPTURE(i, addr);
            REQUIRE(addr % alignof(SIMDVec) == 0);
            REQUIRE(ptr->data[0] == static_cast<float>(i));
            pool.release(ptr);
        }
    }

    SECTION("Alignment is preserved for reused free list slots")
    {
        ncore::PagedObjectPool<SIMDVec> pool(4);
        SIMDVec* a = pool.acquire(1.f);
        pool.release(a);

        SIMDVec* b          = pool.acquire(2.f); // reused slot
        std::uintptr_t addr = reinterpret_cast<std::uintptr_t>(b);
        REQUIRE(addr % alignof(SIMDVec) == 0);
        REQUIRE(b->data[0] == 2.f);
        pool.release(b);
    }
}

// ---------------------------------------------------------------------------
// Safety / invalid input
// ---------------------------------------------------------------------------

TEST_CASE("PagedObjectPool handles invalid release gracefully", "[PagedObjectPool][Safety]")
{

    SECTION("Releasing a pointer not belonging to the pool is a no-op")
    {
        ncore::PagedObjectPool<int> pool(2);
        int* a = pool.acquire(1);
        REQUIRE(pool.get_active_count() == 1);

        int unrelated = 0;
        pool.release(&unrelated); // should assert/no-op, not corrupt state
        REQUIRE(pool.get_active_count() == 1);

        pool.release(a);
        REQUIRE(pool.get_active_count() == 0);
    }

    SECTION("Releasing nullptr is a no-op")
    {
        ncore::PagedObjectPool<int> pool(2);
        int* a = pool.acquire(1);
        pool.release(nullptr);
        REQUIRE(pool.get_active_count() == 1);
        pool.release(a);
        REQUIRE(pool.get_active_count() == 0);
    }
}

TEST_CASE("PagedObjectPool free list remains consistent under stress", "[PagedObjectPool][FreeList]")
{

    SECTION("Interleaved acquire and release keeps active count correct")
    {
        ncore::PagedObjectPool<int64_t> pool(4);

        int64_t* a = pool.acquire(1);
        int64_t* b = pool.acquire(2);
        pool.release(a);              // free list: [a]
        int64_t* c = pool.acquire(3); // reuses a
        REQUIRE(c == a);
        REQUIRE(*c == 3);
        REQUIRE(pool.get_active_count() == 2); // b, c

        int64_t* d = pool.acquire(4);          // new slot
        pool.release(b);                       // free list: [b]
        pool.release(d);                       // free list: [d, b]
        int64_t* e = pool.acquire(5);          // reuses d
        int64_t* f = pool.acquire(6);          // reuses b
        REQUIRE(e == d);
        REQUIRE(f == b);
        REQUIRE(pool.get_active_count() == 3); // c, e, f

        pool.release(c);
        pool.release(e);
        pool.release(f);
        REQUIRE(pool.get_active_count() == 0);
    }

    SECTION("Alternating single acquire/release never grows beyond one page")
    {
        ncore::PagedObjectPool<int64_t> pool(4);

        for (int i = 0; i < 100; ++i) {
            int64_t* p = pool.acquire(i);
            REQUIRE(p != nullptr);
            REQUIRE(*p == i);
            pool.release(p);
        }

        // Only one page should ever have been needed
        REQUIRE(pool.get_page_count() == 1);
        REQUIRE(pool.get_active_count() == 0);
    }

    SECTION("Release all then reacquire all reuses exact same pointers")
    {
        ncore::PagedObjectPool<int64_t> pool(4);
        std::vector<int64_t*> first_wave;

        for (int i = 0; i < 4; ++i)
            first_wave.push_back(pool.acquire(i));

        // Release in forward order — free list will be [3,2,1,0]
        for (int64_t* p : first_wave)
            pool.release(p);

        // Reacquire in reverse order due to LIFO
        std::vector<int64_t*> second_wave;
        for (int i = 0; i < 4; ++i)
            second_wave.push_back(pool.acquire(i + 10));

        // Every pointer must be one of the original slots
        for (int64_t* p : second_wave) {
            bool is_reused = std::find(first_wave.begin(), first_wave.end(), p) != first_wave.end();
            REQUIRE(is_reused);
        }

        REQUIRE(pool.get_page_count() == 1); // no new pages
        REQUIRE(pool.get_active_count() == 4);

        for (int64_t* p : second_wave)
            pool.release(p);
    }

    SECTION("Partial release then grow then release all")
    {
        ncore::PagedObjectPool<int64_t> pool(4);

        int64_t* a = pool.acquire(1);
        int64_t* b = pool.acquire(2);
        int64_t* c = pool.acquire(3);
        int64_t* d = pool.acquire(4);

        pool.release(b);
        pool.release(c);
        // free list: [c, b], pages: 1, active: 2

        // Grow beyond first page
        int64_t* e = pool.acquire(5); // reuses c
        int64_t* f = pool.acquire(6); // reuses b
        int64_t* g = pool.acquire(7); // new slot, may trigger page 2
        int64_t* h = pool.acquire(8); // new slot

        REQUIRE(e == c);
        REQUIRE(f == b);
        REQUIRE(pool.get_active_count() == 6); // a,d,e,f,g,h

        pool.release(a);
        pool.release(d);
        pool.release(e);
        pool.release(f);
        pool.release(g);
        pool.release(h);
        REQUIRE(pool.get_active_count() == 0);
    }
}
