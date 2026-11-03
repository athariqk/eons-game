#pragma once

#include <cstdint>
#include <cstring>
#include <memory>
#include <vector>

#include <ncore/types/typedb.h>
#include <ncore/utils/assert.h>

#include "ecs_component.h"

namespace nc {

/**
 * @brief Represents an entity as integer ID.
 */
using EcsEntity = uint64_t;

inline constexpr EcsEntity INVALID_ENTITY_ID = static_cast<EcsEntity>( -1 );

class EcsWorld;

//------------------------------------------------------------------------------
// EcsEntityBuilder
//------------------------------------------------------------------------------

class NCAPI EcsEntityBuilder {
public:
    EcsEntityBuilder( EcsWorld& p_world NC_LIFETIME_BOUND, const String& p_name );
    EcsEntityBuilder( EcsWorld& p_world NC_LIFETIME_BOUND, EcsEntity p_entity );
    ~EcsEntityBuilder();

    EcsEntityBuilder( const EcsEntityBuilder& )            = delete;
    EcsEntityBuilder& operator=( const EcsEntityBuilder& ) = delete;

    template<class T>
    EcsEntityBuilder& add( const T& value ) NC_LIFETIME_BOUND
    {
        auto* type = TypeDB::find<T>();
        NC_ASSERT_MSG( type, "component type not registered via NC_COMPONENT" );
        DynamicArray<uint8_t> data( sizeof( T ) );
        std::memcpy( data.data(), &value, sizeof( T ) );
        add_component_( type, std::move( data ) );
        return *this;
    }

    /**
     * @brief Append component(s) to the build list.
     */
    template<typename T, typename... Args>
    EcsEntityBuilder& add( Args&&... args ) NC_LIFETIME_BOUND
    {
        auto* type = TypeDB::find<T>();
        NC_ASSERT_MSG( type, "component type not registered via NC_COMPONENT" );
        DynamicArray<uint8_t> data( sizeof( T ) );
        T value{ std::forward<Args>( args )... };
        std::memcpy( data.data(), &value, sizeof( T ) );
        add_component_( type, std::move( data ) );
        return *this;
    }

    /**
     * @brief Append a component pair to the build list.
     */
    template<typename First, typename Second, typename... Args>
    EcsEntityBuilder& add_pair( Args&&... args )
    {
        auto* f_type = TypeDB::find<First>();
        auto* s_type = TypeDB::find<Second>();
        NC_ASSERT_MSG( f_type, "pair first type not registered via NC_COMPONENT" );
        NC_ASSERT_MSG( s_type, "pair second type not registered via NC_COMPONENT" );
        if constexpr (sizeof...( Args ) != 0) {
            DynamicArray<uint8_t> data( sizeof( First ) );
            First value{ std::forward<Args>( args )... };
            std::memcpy( data.data(), &value, sizeof( First ) );
            add_pair_data_( f_type, s_type, std::move( data ) );
        } else {
            add_pair_tag_( f_type, s_type );
        }
        return *this;
    }

    EcsEntityBuilder& add_pair_id( EcsComponent first, EcsComponent second ) NC_LIFETIME_BOUND;
    /**
     * @brief Add parent-child relationship.
     * @param parent The parent entity to set for this entity.
     */
    EcsEntityBuilder& child_of( EcsEntity parent ) NC_LIFETIME_BOUND;
    EcsEntityBuilder& is_a( EcsEntity base ) NC_LIFETIME_BOUND;
    EcsEntityBuilder& depends_on( EcsEntity target ) NC_LIFETIME_BOUND;
    EcsEntityBuilder& alias( StringView alias ) NC_LIFETIME_BOUND;

    /**
     * @brief Mark the most recently added component as initially disabled.
     */
    EcsEntityBuilder& disabled() NC_LIFETIME_BOUND;

    /**
     * @brief Finalize entity creation and set its components, in order.
     */
    EcsEntity build();

private:
    void add_component_( const types::TypeInfo* type, DynamicArray<uint8_t>&& data );
    void add_pair_data_( const types::TypeInfo* first, const types::TypeInfo* second, DynamicArray<uint8_t>&& data );
    void add_pair_tag_( const types::TypeInfo* first, const types::TypeInfo* second );

    struct ComponentEntry {
        const types::TypeInfo* type;
        DynamicArray<uint8_t> data;
        bool disabled = false;
    };
    struct PairEntry {
        EcsComponent first_id;
        EcsComponent second_id;
        const types::TypeInfo* comp_type = nullptr; // if nullptr, means this "component" is tag-only
        DynamicArray<uint8_t> comp_data;
    };

    EcsWorld& world;
    String name;
    EcsEntity id = 0;
    String alias_; // TODO: could be a node path :)
    DynamicArray<ComponentEntry> components;
    DynamicArray<PairEntry> pairs;
    bool built = false;
};

} // namespace nc
