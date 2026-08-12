#pragma once

#include <cstdint>
#include <cstring>
#include <memory>
#include <vector>

#include <ncore/core/types.h>
#include <ncore/utils/assert.h>

namespace nc {

using EcsEntity    = uint64_t;
using EcsComponent = EcsEntity;

inline constexpr EcsEntity INVALID_ENTITY_ID = static_cast<EcsEntity>( -1 );

class EcsWorld;

//------------------------------------------------------------------------------
// EcsEntityBuilder
//------------------------------------------------------------------------------

class NCAPI EcsEntityBuilder {
public:
    EcsEntityBuilder( EcsWorld& p_world, const String& p_name );
    EcsEntityBuilder( EcsWorld& p_world, EcsEntity p_entity );
    ~EcsEntityBuilder();

    EcsEntityBuilder( const EcsEntityBuilder& )            = delete;
    EcsEntityBuilder& operator=( const EcsEntityBuilder& ) = delete;

    template<class T>
    EcsEntityBuilder& add( const T& value )
    {
        auto* type = rtti::TypeRegistry::find<T>();
        NC_ASSERT( type, "component type not reflected via NSTRUCT" );
        DynamicArray<uint8_t> data( sizeof( T ) );
        std::memcpy( data.data(), &value, sizeof( T ) );
        add_component_( type, std::move( data ) );
        return *this;
    }

    /**
     * @brief Append component(s) to the build list.
     */
    template<typename T, typename... Args>
    EcsEntityBuilder& add( Args&&... args )
    {
        auto* type = rtti::TypeRegistry::find<T>();
        NC_ASSERT( type, "component type not reflected via NSTRUCT" );
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
        auto* f_type = rtti::TypeRegistry::find<First>();
        auto* s_type = rtti::TypeRegistry::find<Second>();
        NC_ASSERT( f_type, "pair first type not reflected via NSTRUCT" );
        NC_ASSERT( s_type, "pair second type not reflected via NSTRUCT" );
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

    EcsEntityBuilder& add_pair_id( EcsComponent first, EcsComponent second );
    /**
     * @brief Add parent-child relationship.
     * @param parent The parent entity to set for this entity.
     */
    EcsEntityBuilder& child_of( EcsEntity parent );
    EcsEntityBuilder& is_a( EcsEntity base );
    EcsEntityBuilder& depends_on( EcsEntity target );
    EcsEntityBuilder& alias( StringView alias );

    /**
     * @brief Mark the most recently added component as initially disabled.
     */
    EcsEntityBuilder& disabled();

    /**
     * @brief Finalize entity creation and set its components, in order.
     */
    EcsEntity build();

private:
    void add_component_( const rtti::TypeInfo* type, DynamicArray<uint8_t>&& data );
    void add_pair_data_( const rtti::TypeInfo* first, const rtti::TypeInfo* second, DynamicArray<uint8_t>&& data );
    void add_pair_tag_( const rtti::TypeInfo* first, const rtti::TypeInfo* second );

    struct ComponentEntry {
        const rtti::TypeInfo* type;
        DynamicArray<uint8_t> data;
        bool disabled = false;
    };
    struct PairEntry {
        EcsComponent first_id;
        EcsComponent second_id;
        const rtti::TypeInfo* comp_type = nullptr; // if nullptr, means this "component" is tag-only
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
