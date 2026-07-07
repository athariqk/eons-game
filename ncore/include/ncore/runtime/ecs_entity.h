#pragma once

#include <cstdint>
#include <cstring>
#include <memory>
#include <string>
#include <string_view>
#include <vector>

#include <ncore/kernel/types.h>
#include <ncore/utils/assert.h>

namespace nc {

using EcsEntityId    = uint64_t;
using EcsComponentId = EcsEntityId;

inline constexpr EcsEntityId INVALID_ENTITY_ID = static_cast<EcsEntityId>( -1 );

class EcsWorld;

//------------------------------------------------------------------------------
// EcsEntityBuilder
//------------------------------------------------------------------------------

class NCORE_API EcsEntityBuilder {
public:
    EcsEntityBuilder( EcsWorld& world, const std::string& name );
    ~EcsEntityBuilder();

    EcsEntityBuilder( const EcsEntityBuilder& )            = delete;
    EcsEntityBuilder& operator=( const EcsEntityBuilder& ) = delete;

    template<typename T, typename... Args>
    EcsEntityBuilder& with( Args&&... args )
    {
        auto* type = rfl::Registry::find<T>();
        NC_ASSERT( type, "component type not reflected via NSTRUCT" );
        std::vector<uint8_t> data( sizeof( T ) );
        T value{ std::forward<Args>( args )... };
        std::memcpy( data.data(), &value, sizeof( T ) );
        add_component_( type, std::move( data ) );
        return *this;
    }

    template<typename First, typename Second, typename... Args>
    EcsEntityBuilder& with_pair( Args&&... args )
    {
        auto* f_type = rfl::Registry::find<First>();
        auto* s_type = rfl::Registry::find<Second>();
        NC_ASSERT( f_type, "pair first type not reflected via NSTRUCT" );
        NC_ASSERT( s_type, "pair second type not reflected via NSTRUCT" );
        std::vector<uint8_t> data( sizeof( First ) );
        First value{ std::forward<Args>( args )... };
        std::memcpy( data.data(), &value, sizeof( First ) );
        add_pair_data_( f_type, s_type, std::move( data ) );
        return *this;
    }

    template<typename First, typename Second>
    EcsEntityBuilder& add_pair()
    {
        auto* f_type = rfl::Registry::find<First>();
        auto* s_type = rfl::Registry::find<Second>();
        NC_ASSERT( f_type, "pair first type not reflected via NSTRUCT" );
        NC_ASSERT( s_type, "pair second type not reflected via NSTRUCT" );
        add_pair_tag_( f_type, s_type );
        return *this;
    }

    EcsEntityBuilder& add_pair_id( EcsComponentId first, EcsComponentId second );
    EcsEntityBuilder& child_of( EcsEntityId parent );
    EcsEntityBuilder& is_a( EcsEntityId base );
    EcsEntityBuilder& depends_on( EcsEntityId target );
    EcsEntityBuilder& alias( std::string_view alias );

    EcsEntityId build();

private:
    void add_component_( const rfl::TypeInfo* type, std::vector<uint8_t>&& data );
    void add_pair_data_( const rfl::TypeInfo* first, const rfl::TypeInfo* second, std::vector<uint8_t>&& data );
    void add_pair_tag_( const rfl::TypeInfo* first, const rfl::TypeInfo* second );

    struct Impl;
    std::unique_ptr<Impl> pImpl;
};

} // namespace nc
