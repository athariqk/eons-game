#pragma once

#include <cstdint>
#include <map>
#include <memory>
#include <unordered_map>

#include <ncore/core/types.h>
#include <ncore/runtime/ecs/ecs_entity.h>

namespace nc {

class EcsWorld;
class EcsQueryBuilder;
class ServiceRegistry;

//------------------------------------------------------------------------------

class EcsIterator {
public:
    EcsIterator() = default;
    ~EcsIterator();

    /**
     * @brief Wraps an implementation detail iterator object (Flecs' ecs_iter_t).
     */
    EcsIterator( void* internal ) noexcept;

    EcsIterator( EcsIterator&& ) noexcept;
    EcsIterator& operator=( EcsIterator&& ) noexcept;
    EcsIterator( const EcsIterator& )            = delete;
    EcsIterator& operator=( const EcsIterator& ) = delete;

    bool operator!=( std::nullptr_t ) const;
    EcsIterator& operator++();

    double delta_time() const;
    float delta_time_internal() const;
    int32_t count() const;
    EcsEntity entity( int32_t row ) const;
    EcsWorld& world() const;
    EcsEntity event();
    void* event_payload();
    void* user_ctx() const;

    void* get_internal_iter() { return iter_; }

private:
    friend class EcsQuery;

    enum class Kind : uint8_t { Query };

    EcsIterator( EcsWorld* world_ref, void* world, void* query );

    void* world_ = nullptr;
    void* query_ = nullptr;
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
    EcsQuery() = default;

    /**
     * @brief Return iterator for queried tables.
     */
    EcsIterator begin();
    static std::nullptr_t end();

private:
    friend class EcsWorld;
    friend class EcsQueryBuilder;
    friend class EcsSystemBuilder;

    EcsQuery( EcsWorld* world_ref, void* world_handle, void* query_handle );

    // internal impl details
    EcsWorld* world_ref_ = nullptr;
    void* world_         = nullptr; // native world
    void* query_         = nullptr; // native query
};

//------------------------------------------------------------------------------

/**
 * @brief QueryContext represents the current iteration state of a query.
 *
 * TODO: this feels too tied to Flecs' table-based storage
 */
class NCAPI QueryContext {
public:
    explicit QueryContext( void* iter );

    double delta_time() const;
    float delta_time_internal() const;
    int32_t count() const;
    EcsEntity entity( int32_t row ) const;
    EcsWorld& world() const;
    EcsEntity event();
    void* event_payload();
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

    template<typename T>
    T* get_component()
    {
        static const auto& info = rtti::TypeRegistry::get<T>();
        int32_t idx             = resolve_term_index_( info );
        NC_ASSERT( idx >= 0, "Component not found in query terms" );
        return static_cast<T*>( get_component_( idx, info.size, info.alignment ) );
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
    int32_t resolve_term_index_( const rtti::TypeInfo& info ) const;
    int32_t resolve_pair_index_( const rtti::TypeInfo& first, const rtti::TypeInfo& second ) const;
    void* it_            = nullptr;
    int32_t current_row_ = 0;

    mutable std::unordered_map<const rtti::TypeInfo*, int32_t> term_cache_;
    mutable std::map<std::pair<const rtti::TypeInfo*, const rtti::TypeInfo*>, int32_t> pair_cache_;
};

//------------------------------------------------------------------------------

class NCAPI EcsQueryBuilder {
public:
    EcsQueryBuilder( EcsWorld& world, String name );
    ~EcsQueryBuilder();

    EcsQueryBuilder( const EcsQueryBuilder& )            = delete;
    EcsQueryBuilder& operator=( const EcsQueryBuilder& ) = delete;

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
     * @brief Set up traversal on the last added term (default: ChildOf).
     */
    EcsQueryBuilder& up();

    /**
     * @brief Match the last term on self (default, explicit for clarity).
     */
    EcsQueryBuilder& self();

    /**
     * @brief Set the optional query DSL expression.
     */
    EcsQueryBuilder& expr( StringView dsl );

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
