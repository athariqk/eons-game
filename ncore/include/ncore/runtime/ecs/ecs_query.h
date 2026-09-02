#pragma once

#include <cstddef>
#include <cstdint>
#include <iterator>
#include <map>
#include <memory>
#include <unordered_map>

#include <ncore/core/types.h>
#include <ncore/runtime/ecs/ecs_entity.h>

namespace nc {

class EcsWorld;
class EcsQueryBuilder;
class EcsEntityView;
class ServiceRegistry;

//------------------------------------------------------------------------------

/**
 * @brief Table-level input iterator over the archetypes matched by an EcsQuery.
 */
class NCAPI EcsTableIterator {
public:
    using iterator_category = std::input_iterator_tag;
    using value_type        = EcsTableIterator;
    using difference_type   = std::ptrdiff_t;
    using pointer           = EcsTableIterator*;
    using reference         = EcsTableIterator&;

    EcsTableIterator() = default;
    ~EcsTableIterator();

    /**
     * @brief Wraps an implementation detail iterator object (Flecs' ecs_iter_t).
     */
    EcsTableIterator( void* internal ) noexcept;

    EcsTableIterator( EcsTableIterator&& ) noexcept;
    EcsTableIterator& operator=( EcsTableIterator&& ) noexcept;
    EcsTableIterator( const EcsTableIterator& )            = delete;
    EcsTableIterator& operator=( const EcsTableIterator& ) = delete;

    reference operator*() noexcept
    {
        return *this;
    }
    const EcsTableIterator& operator*() const noexcept
    {
        return *this;
    }

    pointer operator->() noexcept
    {
        return this;
    }
    const EcsTableIterator* operator->() const noexcept
    {
        return this;
    }

    bool operator==( const EcsTableIterator& o ) const noexcept
    {
        return done_ == o.done_;
    }
    bool operator!=( const EcsTableIterator& o ) const noexcept
    {
        return done_ != o.done_;
    }

    EcsTableIterator& operator++();

    bool is_done() const noexcept
    {
        return done_;
    }

    double delta_time() const;
    float delta_time_internal() const;
    int32_t count() const;
    EcsEntity entity( int32_t row ) const;
    EcsWorld& world() const;
    EcsEntity event();
    void* event_payload();
    void* user_ctx() const;

    void* get_internal_iter() const
    {
        return iter_;
    }

private:
    friend class EcsQuery;

    enum class Kind : uint8_t {
        Query
    };

    EcsTableIterator( EcsWorld* world_ref, void* world, void* query );

    void* world_ = nullptr;
    void* query  = nullptr;
    void* iter_  = nullptr;
    Kind kind_   = Kind::Query;
    bool done_   = true;
};

//------------------------------------------------------------------------------

/**
 * @brief A lightweight, non-owning handle to a query owned by an EcsWorld.
 *
 * This is cached and supports range-for iteration.
 *
 * Currently, we mostly just have wrappers for Flecs C API.
 * Just something that we can build upon in the future
 * from a standardized base.
 */
class NCAPI EcsQuery {
public:
    EcsQuery() : name(), world_ref( nullptr ), world( nullptr ), query( nullptr ) {}

    /**
     * @brief Return iterator for queried tables.
     */
    EcsTableIterator begin();
    EcsTableIterator end();

    /**
     * @brief Return a per-entity view over the query (range-for compatible).
     *
     * Yields a EcsIterState for every matched entity, across all tables.
     */
    EcsEntityView entities();

    StringView get_name()
    {
        return name;
    }

    /**
     * @brief Checks if a query handle is not null and is returning results.
     */
    bool is_valid();

private:
    friend class EcsWorld;
    friend class EcsQueryBuilder;
    friend class EcsSystemBuilder;

    EcsQuery( const String& name, EcsWorld* world_ref, void* world_handle, void* query_handle );

    String name;
    // internal impl details
    EcsWorld* world_ref = nullptr;
    void* world         = nullptr; // native world
    void* query         = nullptr; // native query
};

//------------------------------------------------------------------------------

/**
 * @brief EcsIterState represents the current state of matched entity.
 *
 * TODO: this feels too tied to Flecs' table-based storage
 */
class NCAPI EcsIterState {
public:
    EcsIterState() = default;
    explicit EcsIterState( void* iter );

    double delta_time() const;             // The global delta time.
    float delta_time_internal() const;     // This iter's own delta time.
    int32_t count() const;                 // The entity count being iterated.
    EcsEntity entity() const;              // The current entity.
    EcsEntity entity( int32_t row ) const; // Gets entity ID at given row.
    EcsWorld& world() const;               // The current world (staged).
    EcsEntity event();                     // Returns any event if applicable.
    void* event_payload();                 // Returns the event payload if applicable.
    void* user_ctx() const;

    template<typename T>
    T* event_payload()
    {
        return reinterpret_cast<T*>( event_payload() );
    }

    /**
     * @brief Sets the component instance that will be returned by get_component().
     */
    void set_row( int32_t row )
    {
        current_row_ = row;
    }

    /**
     * @brief Retrieve the component in this iteration by type.
     */
    template<typename T>
    T* get_component()
    {
        static const auto& info = rtti::TypeRegistry::get<T>();
        int32_t idx             = resolve_term_index_( info );
        NC_ASSERT( idx >= 0, "Component not found in query terms" );
        return static_cast<T*>( get_component_( idx, info.size, info.alignment ) );
    }

    /**
     * @brief Retrieve the component by term index.
     */
    template<typename T>
    T* get_component( uint8_t column )
    {
        static const auto& info = rtti::TypeRegistry::get<T>();
        return static_cast<T*>( get_component_( column, info.size, info.alignment ) );
    }

    template<typename T>
    void mark_component_modified()
    {
        static const auto& info = rtti::TypeRegistry::get<T>();
        mark_component_modified_( &info );
    }

    template<typename First, typename Second>
    First* get_pair()
    {
        static const auto& first_info  = rtti::TypeRegistry::get<First>();
        static const auto& second_info = rtti::TypeRegistry::get<Second>();
        int32_t idx                    = resolve_pair_index_( first_info, second_info );
        NC_ASSERT( idx >= 0, "Pair component not found in query terms" );
        return static_cast<First*>( get_component_( idx, first_info.size, first_info.alignment ) );
    }

private:
    friend class EcsSystemBuilder;
    friend class EcsObserverBuilder;
    friend class EcsQuery;

    void* get_component_( int32_t column, size_t size, size_t alignment ) const;
    void mark_component_modified_( const rtti::TypeInfo* type ) const;
    int32_t resolve_term_index_( const rtti::TypeInfo& info ) const;
    int32_t resolve_pair_index_( const rtti::TypeInfo& first, const rtti::TypeInfo& second ) const;
    void* it_            = nullptr;
    int32_t current_row_ = 0;

    mutable std::unordered_map<const rtti::TypeInfo*, int32_t> term_cache_;
    mutable std::map<std::pair<const rtti::TypeInfo*, const rtti::TypeInfo*>, int32_t> pair_cache_;
};

//------------------------------------------------------------------------------

/**
 * @brief Per-entity input iterator over the entities matched by an EcsQuery.
 *
 * Walks rows across tables (archetypes). Dereferencing yields a copyable
 * EcsIterState with the current row pre-set. Equality is only meaningful
 * against the end sentinel (default-constructed EcsEntityIterator).
 */
class NCAPI EcsEntityIterator {
public:
    using iterator_category = std::input_iterator_tag;
    using value_type        = EcsIterState;
    using difference_type   = std::ptrdiff_t;
    using pointer           = void;
    using reference         = EcsIterState;

    EcsEntityIterator() = default;

    explicit EcsEntityIterator( EcsTableIterator table );

    EcsIterState operator*() const
    {
        return ctx_;
    }

    const EcsIterState* operator->() const
    {
        return &ctx_;
    }

    EcsEntityIterator& operator++();

    bool operator==( const EcsEntityIterator& o ) const noexcept
    {
        return done_ == o.done_;
    }
    bool operator!=( const EcsEntityIterator& o ) const noexcept
    {
        return done_ != o.done_;
    }

private:
    EcsTableIterator table_;
    EcsIterState ctx_;
    int32_t row_ = 0;
    bool done_   = true;
};

/**
 * @brief Lightweight per-entity range view over an EcsQuery.
 */
class NCAPI EcsEntityView {
public:
    explicit EcsEntityView( EcsQuery& query );

    EcsEntityIterator begin();
    EcsEntityIterator end();

private:
    EcsQuery* query_;
};

//------------------------------------------------------------------------------

class NCAPI EcsQueryBuilder {
public:
    EcsQueryBuilder( EcsWorld& world, String name );
    ~EcsQueryBuilder();

    EcsQueryBuilder( const EcsQueryBuilder& )            = delete;
    EcsQueryBuilder& operator=( const EcsQueryBuilder& ) = delete;

    /**
     * @brief Add component to query term.
     */
    template<typename... Comps>
    EcsQueryBuilder& with()
    {
        ( add_term_<Comps>( 0 ), ... );
        return *this;
    }

    template<typename First, typename Second>
    EcsQueryBuilder& with_pair()
    {
        add_term_pair_impl( rtti::TypeRegistry::find<First>(), rtti::TypeRegistry::find<Second>(), 0 );
        return *this;
    }

    EcsQueryBuilder& with_pair( EcsEntity first, EcsEntity second );

    template<typename... Comps>
    EcsQueryBuilder& read()
    {
        ( add_term_<Comps>( 1 ), ... );
        return *this;
    }

    /**
     * @brief Match on all components with read/write.
     */
    EcsQueryBuilder& all();

    /**
     * @brief Match on all components with read-only.
     */
    EcsQueryBuilder& all_read();

    /**
     * @brief Traverse relationship bottom-up.
     *
     * Set up traversal on the last added term (default: ChildOf).
     * TODO: allow custom relationship traversal
     */
    EcsQueryBuilder& up();

    /**
     * @brief Match the last term on self (default, explicit for clarity).
     */
    EcsQueryBuilder& self();

    /**
     * @brief Exclude self from the last term's match (walk parent only, used with up()).
     */
    EcsQueryBuilder& skip_self();

    /**
     * @brief Traverse relationship top-down.
     *
     * Order results breadth-first through the ChildOf hierarchy (cascade).
     */
    EcsQueryBuilder& cascade();

    /**
     * @brief Set the optional query DSL expression.
     */
    EcsQueryBuilder& expr( StringView dsl );

	EcsQueryBuilder& src( EcsEntity id );

    /**
     * @brief Returns the query name.
     */
    const String& name() const;

    /**
     * @brief Finalise and build the query.
     */
    EcsQuery build();

private:
    friend class EcsSystemBuilder;
    friend class EcsObserverBuilder;

    template<typename T>
    void add_term_( uint8_t inout )
    {
        add_term_impl( rtti::TypeRegistry::find<T>(), inout );
    }
    void add_term_impl( const rtti::TypeInfo* type, uint8_t inout );
    void add_term_pair_impl( const rtti::TypeInfo* first_type, const rtti::TypeInfo* sec_type, uint8_t inout );

    struct Impl;
    std::unique_ptr<Impl> pImpl;
};

} // namespace nc
