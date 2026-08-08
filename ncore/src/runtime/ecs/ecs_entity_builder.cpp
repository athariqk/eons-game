#include <flecs.h>

#include <ncore/runtime/ecs/ecs_entity.h>
#include <ncore/runtime/ecs/ecs_world.h>
#include <ncore/utils/log.h>

namespace nc {

struct EcsEntityBuilder::Impl {
    struct ComponentEntry {
        const rtti::TypeInfo* type;
        DynamicArray<uint8_t> data;
    };
    struct PairEntry {
        EcsComponent first_id;
        EcsComponent second_id;
        const rtti::TypeInfo* comp_type = nullptr; // nullptr -> tag-only
        DynamicArray<uint8_t> comp_data;
    };

    EcsWorld& world;
    String name;
    String alias_; // TODO: could be a node path :)
    DynamicArray<ComponentEntry> components;
    DynamicArray<PairEntry> pairs;
    bool built = false;
};

//------------------------------------------------------------------------------

EcsEntityBuilder::EcsEntityBuilder( EcsWorld& world, const String& name ) :
    pImpl( std::make_unique<Impl>( world, name ) )
{}

EcsEntityBuilder::~EcsEntityBuilder()
{
    if (!pImpl->built) {
        NC_LOG_WARN_C( log::ECS, "EcsEntityBuilder discarded without build" );
    }
}

//------------------------------------------------------------------------------

void EcsEntityBuilder::add_component_( const rtti::TypeInfo* type, DynamicArray<uint8_t>&& data )
{
    pImpl->components.push_back( { type, std::move( data ) } );
}

void EcsEntityBuilder::add_pair_data_(
    const rtti::TypeInfo* first, const rtti::TypeInfo* second, DynamicArray<uint8_t>&& data
)
{
    EcsComponent f_id = pImpl->world.register_component_type( first );
    EcsComponent s_id = pImpl->world.register_component_type( second );
    pImpl->pairs.push_back( { f_id, s_id, first, std::move( data ) } );
}

void EcsEntityBuilder::add_pair_tag_( const rtti::TypeInfo* first, const rtti::TypeInfo* second )
{
    EcsComponent f_id = pImpl->world.register_component_type( first );
    EcsComponent s_id = pImpl->world.register_component_type( second );
    pImpl->pairs.push_back( { f_id, s_id, nullptr, {} } );
}

EcsEntityBuilder& EcsEntityBuilder::add_pair_id( EcsComponent first, EcsComponent second )
{
    pImpl->pairs.push_back( { first, second, nullptr, {} } );
    return *this;
}

EcsEntityBuilder& EcsEntityBuilder::child_of( EcsEntity parent )
{
    return add_pair_id( static_cast<EcsComponent>( EcsChildOf ), parent );
}

EcsEntityBuilder& EcsEntityBuilder::is_a( EcsEntity base )
{
    return add_pair_id( static_cast<EcsComponent>( EcsIsA ), base );
}

EcsEntityBuilder& EcsEntityBuilder::depends_on( EcsEntity target )
{
    return add_pair_id( static_cast<EcsComponent>( EcsDependsOn ), target );
}

EcsEntityBuilder& EcsEntityBuilder::alias( StringView alias )
{
    pImpl->alias_ = alias;
    return *this;
}

//------------------------------------------------------------------------------

EcsEntity EcsEntityBuilder::build()
{
    auto world = static_cast<ecs_world_t*>( pImpl->world.get_native_handle() );

    // create bare entity
    ecs_entity_desc_t desc{};
    if (!pImpl->name.empty())
        desc.name = pImpl->name.c_str();
    if (!pImpl->alias_.empty())
        desc.symbol = pImpl->alias_.c_str(); // TODO: rectify this
    ecs_entity_t ent = ecs_entity_init( world, &desc );
    NC_ASSERT( ent != 0, "Failed to create entity" );

    // set components
    for (auto& c : pImpl->components) {
        EcsComponent comp_id = pImpl->world.register_component_type( c.type );
        ecs_set_id( world, ent, comp_id, c.data.size(), c.data.data() );
    }

    // add pairs
    for (auto& p : pImpl->pairs) {
        ecs_id_t pair_id = ecs_make_pair( p.first_id, p.second_id );
        if (p.comp_type) {
            ecs_set_id( world, ent, pair_id, p.comp_data.size(), p.comp_data.data() );
        } else {
            ecs_add_id( world, ent, pair_id );
        }
    }

    pImpl->built = true;
    NC_LOG_DEBUG_C( log::ECS, "Built entity '{}' (id {})", pImpl->name, ent );
    return static_cast<EcsEntity>( ent );
}

} // namespace nc
