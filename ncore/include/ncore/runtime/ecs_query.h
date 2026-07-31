#pragma once

#include <cstdint>
#include <map>
#include <memory>
#include <string>
#include <unordered_map>

#include <ncore/core/types.h>
#include <ncore/runtime/ecs_entity.h>

namespace nc {

class EcsWorld;
class EcsQueryBuilder;
class ModuleRegistry;

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
    class NCAPI Iterator;

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
 * @brief QueryContext represents the current iteration state of a query.
 *
 * TODO: this feels too tied to Flecs' table-based storage
 */
class NCAPI QueryContext {
public:
    explicit QueryContext( void* iter );

    double delta_time() const;               // The global delta time.
    float delta_time_internal() const;       // System's own delta time.
    int32_t count() const;                   // The entity count being iterated.
    EcsEntityId entity( int32_t row ) const; // Gets entity ID at given row.
    EcsWorld& world() const;                 // The current world.
    ModuleRegistry& modules() const;         // Helper access to engine modules.
    EcsEntityId event();                     // Returns any event if applicable.
    void* event_payload();

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
     * @brief Typed component access by type. Scans the iterator's field IDs
     * to find the matching term index. Preferred over the index-based overload
     * as it is immune to term reordering.
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
     * @brief Typed access to pair component data by pair type.
     */
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

    // iterator impl details
    void set_iter_( void* iter );
    void* get_component_( int32_t column, size_t size, size_t alignment ) const;
    int32_t resolve_term_index_( const rtti::TypeInfo& info ) const;
    int32_t resolve_pair_index_( const rtti::TypeInfo& first, const rtti::TypeInfo& second ) const;
    void* it_            = nullptr;
    int32_t current_row_ = 0;

    // cache resolved term/pair indices per (table batch) to avoid redundant
    // register_component_type calls on every get_component<T>()
    mutable std::unordered_map<const rtti::TypeInfo*, int32_t> term_cache_;
    mutable std::map<std::pair<const rtti::TypeInfo*, const rtti::TypeInfo*>, int32_t> pair_cache_;
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
    QueryContext& operator*();

private:
    friend class EcsQuery;
    Iterator( EcsWorld* world_ref, void* world, void* query );

    // internal impl details
    void* world_ = nullptr;
    void* query_ = nullptr;
    void* iter_  = nullptr;
    QueryContext wrapper_{ nullptr };
    bool done_ = true;
};

//------------------------------------------------------------------------------

class NCAPI EcsQueryBuilder {
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
        add_term_pair_impl( rtti::TypeRegistry::find<First>(), rtti::TypeRegistry::find<Second>(), 0 );
        return *this;
    }

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
