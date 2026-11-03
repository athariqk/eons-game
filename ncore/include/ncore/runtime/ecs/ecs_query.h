#pragma once

#include <cstddef>
#include <cstdint>
#include <iterator>
#include <map>
#include <memory>
#include <unordered_map>

#include <ncore/runtime/ecs/ecs_entity.h>
#include <ncore/types/typedb.h>

namespace nc {

class EcsWorld;
class EcsQueryBuilder;
class EcsEntityView;
class ServiceDB;

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
    EcsTableIterator( void* internal NC_LIFETIME_BOUND ) noexcept;

    EcsTableIterator( EcsTableIterator&& ) noexcept;
    EcsTableIterator& operator=( EcsTableIterator&& ) noexcept;
    EcsTableIterator( const EcsTableIterator& )            = delete;
    EcsTableIterator& operator=( const EcsTableIterator& ) = delete;

    reference operator*() noexcept NC_LIFETIME_BOUND
    {
        return *this;
    }
    const EcsTableIterator& operator*() const noexcept NC_LIFETIME_BOUND
    {
        return *this;
    }

    pointer operator->() noexcept NC_LIFETIME_BOUND
    {
        return this;
    }
    const EcsTableIterator* operator->() const noexcept NC_LIFETIME_BOUND
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

    EcsTableIterator& operator++() NC_LIFETIME_BOUND;

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

    EcsTableIterator( EcsWorld* world_ref, void* world NC_LIFETIME_BOUND, void* query NC_LIFETIME_BOUND );

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
    EcsEntityView entities() NC_LIFETIME_BOUND;

    StringView get_name() NC_LIFETIME_BOUND
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

    EcsQuery(
        const String& name, EcsWorld* world_ref NC_LIFETIME_BOUND, void* world_handle NC_LIFETIME_BOUND,
        void* query_handle NC_LIFETIME_BOUND
    );

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
    explicit EcsIterState( NC_LIFETIME_BOUND void* iter );

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
        static const auto& info = TypeDB::get<T>();
        int32_t idx             = resolve_term_index_impl( info );
        NC_ASSERT_MSG( idx >= 0, "Component not found in query terms" );
        return static_cast<T*>( get_component_impl( idx, info.size, info.alignment ) );
    }

    /**
     * @brief Retrieve the component by term index.
     */
    template<typename T>
    T* get_component( uint8_t column )
    {
        static const auto& info = TypeDB::get<T>();
        return static_cast<T*>( get_component_impl( column, info.size, info.alignment ) );
    }

    /**
     * @brief Retrieve a component by entity, bypassing term fields.
     *
     * WARNING: may be invisible to the scheduler, mutating T races with systems using it.
     */
    template<typename T>
    T* get_entity_component()
    {
        static const auto* info = TypeDB::find<T>();
        return static_cast<T*>( get_entity_component_impl( info ) );
    }

    /**
     * @brief Signal that a component has been modified for the given entity.
     */
    template<typename T>
    void notify_changed( EcsEntity p_entity )
    {
        static const auto& info = TypeDB::get<T>();
        notify_changed_impl( p_entity, &info );
    }

    /**
     * @brief Signal that a component has been modified for the current entity.
     */
    template<typename T>
    void notify_changed()
    {
        notify_changed<T>( entity() );
    }

    template<typename First, typename Second>
    First* get_pair()
    {
        static const auto& first_info  = TypeDB::get<First>();
        static const auto& second_info = TypeDB::get<Second>();
        int32_t idx                    = resolve_pair_index_impl( first_info, second_info );
        NC_ASSERT_MSG( idx >= 0, "Pair component not found in query terms" );
        return static_cast<First*>( get_component_impl( idx, first_info.size, first_info.alignment ) );
    }

private:
    friend class EcsSystemBuilder;
    friend class EcsObserverBuilder;
    friend class EcsQuery;

    void* get_component_impl( int32_t column, size_t size, size_t alignment ) const;
    void* get_entity_component_impl( const types::TypeInfo* p_type ) const;
    void notify_changed_impl( EcsEntity p_entity, const types::TypeInfo* type ) const;
    int32_t resolve_term_index_impl( const types::TypeInfo& info ) const;
    int32_t resolve_pair_index_impl( const types::TypeInfo& first, const types::TypeInfo& second ) const;
    void* it_            = nullptr;
    int32_t current_row_ = 0;

    mutable std::unordered_map<const types::TypeInfo*, int32_t> term_cache_;
    mutable std::map<std::pair<const types::TypeInfo*, const types::TypeInfo*>, int32_t> pair_cache_;
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

    const EcsIterState* operator->() const NC_LIFETIME_BOUND
    {
        return &ctx_;
    }

    EcsEntityIterator& operator++() NC_LIFETIME_BOUND;

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
    explicit EcsEntityView( NC_LIFETIME_BOUND EcsQuery& query );

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
    EcsQueryBuilder& with() NC_LIFETIME_BOUND
    {
        ( add_term_<Comps>( 0 ), ... );
        return *this;
    }

    template<typename First, typename Second>
    EcsQueryBuilder& with_pair() NC_LIFETIME_BOUND
    {
        add_term_pair_impl( TypeDB::find<First>(), TypeDB::find<Second>(), 0 );
        return *this;
    }

    EcsQueryBuilder& with_pair( EcsEntity first, EcsEntity second ) NC_LIFETIME_BOUND;

    template<typename... Comps>
    EcsQueryBuilder& without() NC_LIFETIME_BOUND
    {
        ( add_term_without_<Comps>(), ... );
        return *this;
    }

    template<typename First, typename Second>
    EcsQueryBuilder& without_pair() NC_LIFETIME_BOUND
    {
        add_term_pair_without_impl( TypeDB::find<First>(), TypeDB::find<Second>() );
        return *this;
    }

    template<typename... Comps>
    EcsQueryBuilder& read() NC_LIFETIME_BOUND
    {
        ( add_term_<Comps>( 1 ), ... );
        return *this;
    }

    /**
     * @brief Match on all components with read/write.
     */
    EcsQueryBuilder& all() NC_LIFETIME_BOUND;

    /**
     * @brief Match on all components with read-only.
     */
    EcsQueryBuilder& all_read() NC_LIFETIME_BOUND;

    /**
     * @brief Traverse relationship bottom-up.
     *
     * Set up traversal on the last added term (default: ChildOf).
     * TODO: allow custom relationship traversal
     */
    EcsQueryBuilder& up() NC_LIFETIME_BOUND;

    /**
     * @brief Match the last term on self (default, explicit for clarity).
     */
    EcsQueryBuilder& self() NC_LIFETIME_BOUND;

    /**
     * @brief Exclude self from the last term's match (walk parent only, used with up()).
     */
    EcsQueryBuilder& skip_self() NC_LIFETIME_BOUND;

    /**
     * @brief Traverse relationship top-down.
     *
     * Order results breadth-first through the ChildOf hierarchy (cascade).
     */
    EcsQueryBuilder& cascade() NC_LIFETIME_BOUND;

    EcsQueryBuilder& filter() NC_LIFETIME_BOUND;

    /**
     * @brief Set the optional query DSL expression.
     */
    EcsQueryBuilder& expr( StringView dsl ) NC_LIFETIME_BOUND;

    EcsQueryBuilder& src( EcsEntity id ) NC_LIFETIME_BOUND;

    /**
     * @brief Returns the query name.
     */
    const String& name() const NC_LIFETIME_BOUND;

    /**
     * @brief Finalise and build the query.
     */
    EcsQuery build();

private:
    friend class EcsSystemBuilder;
    friend class EcsObserverBuilder;

    void add_term_impl( const types::TypeInfo* type, uint8_t inout );
    void add_term_pair_impl( const types::TypeInfo* first_type, const types::TypeInfo* sec_type, uint8_t inout );

    void add_term_without_impl( const types::TypeInfo* type );
    void add_term_pair_without_impl( const types::TypeInfo* first, const types::TypeInfo* second );

    template<typename T>
    void add_term_( uint8_t inout )
    {
        add_term_impl( TypeDB::find<T>(), inout );
    }

    template<typename T>
    void add_term_without_()
    {
        add_term_without_impl( TypeDB::find<T>() );
    }

    struct Impl;
    Ptr<Impl> pImpl;
};

} // namespace nc
