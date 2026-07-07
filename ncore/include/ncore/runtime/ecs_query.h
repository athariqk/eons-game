#pragma once

#include <cstdint>
#include <memory>
#include <string>

#include <ncore/kernel/types.h>
#include <ncore/runtime/ecs_entity.h>

namespace nc {

// Currently, we mostly just have wrappers for Flecs C API.
// Just something that we can build upon in the future
// from a standardized base.

class EcsWorld;
class EcsQueryBuilder;

/**
 * @brief A lightweight, non-owning handle to a query owned by an EcsWorld.
 *
 * This is cached and supports range-for iteration.
 */
class NCORE_API EcsQuery {
public:
    class NCORE_API Iterator;

    EcsQuery() = default;

    Iterator begin();
    static std::nullptr_t end()
    {
        return nullptr;
    }

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
 * @brief EcsIter represents the current iteration state of a query.
 *
 TODO: this feels too tied to Flecs' table-based storage
 */
class NCORE_API EcsIter {
public:
    double delta_time() const;         // The global delta time
    float delta_time_internal() const; // System's own delta time
    int32_t count() const;             // The entity count being iterated
    EcsEntityId entity( int32_t row ) const;
    EcsWorld& world() const;

    /**
     * @brief Typed component access. Component indices are 0-based in the order
     * terms were added to the builder.
     */
    template<typename T>
    T* get_component( int32_t column )
    {
        auto& info = rfl::Registry::get<T>();
        return static_cast<T*>( get_component_( column, info.size, info.alignment ) );
    }

    /**
     * @brief Gets a single element from a typed component at a given row.
     */
    template<typename T>
    T& get_component( int32_t column, int32_t row )
    {
        return get_component<T>( column )[row];
    }

private:
    friend class EcsSystemBuilder;
    friend class EcsQuery;

    // iterator impl details
    explicit EcsIter( void* iter );
    void set_iter_( void* iter );
    void* get_component_( int32_t column, size_t size, size_t alignment ) const;
    void* it_ = nullptr;
};

//------------------------------------------------------------------------------

class EcsQuery::Iterator {
public:
    ~Iterator();

    Iterator( Iterator&& ) noexcept;
    Iterator& operator=( Iterator&& ) noexcept;
    Iterator( const Iterator& )            = delete;
    Iterator& operator=( const Iterator& ) = delete;

    bool operator!=( std::nullptr_t ) const;
    Iterator& operator++();
    EcsIter& operator*();

private:
    friend class EcsQuery;
    Iterator( EcsWorld* world_ref, void* world, void* query );

    // internal impl details
    void* world_ = nullptr;
    void* query_ = nullptr;
    void* iter_  = nullptr;
    EcsIter wrapper_{ nullptr };
    bool done_ = true;
};

//------------------------------------------------------------------------------

class NCORE_API EcsQueryBuilder {
public:
    EcsQueryBuilder( EcsWorld& world, std::string name );
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
        add_term_pair_impl( rfl::Registry::find<First>(), rfl::Registry::find<Second>(), 0 );
        return *this;
    }

    template<typename... Comps>
    EcsQueryBuilder& read()
    {
        ( add_term_<Comps>( 1 ), ... );
        return *this;
    }

    EcsQueryBuilder& all();
    EcsQueryBuilder& all_read();

    /**
     * @brief Set the optional query DSL expression.
     */
    EcsQueryBuilder& expr( std::string_view dsl );

    /**
     * @brief Returns the query name.
     */
    const std::string& name() const;

    /**
     * @brief Finalise and build the query.
     */
    EcsQuery build();

private:
    friend class EcsSystemBuilder;

    template<typename T>
    void add_term_( uint8_t inout )
    {
        add_term_impl( rfl::Registry::find<T>(), inout );
    }
    void add_term_impl( const rfl::TypeInfo* type, uint8_t inout );
    void add_term_pair_impl( const rfl::TypeInfo* first_type, const rfl::TypeInfo* sec_type, uint8_t inout );

    struct Impl;
    std::unique_ptr<Impl> pImpl;
};

} // namespace nc
