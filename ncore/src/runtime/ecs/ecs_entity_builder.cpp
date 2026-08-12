#include <flecs.h>

#include <ncore/runtime/ecs/ecs_entity.h>
#include <ncore/runtime/ecs/ecs_world.h>
#include <ncore/utils/log.h>

namespace nc {

//------------------------------------------------------------------------------

EcsEntityBuilder::EcsEntityBuilder( EcsWorld& p_world, const String& p_name ) : world( p_world ), name( p_name ) {}

EcsEntityBuilder::EcsEntityBuilder( EcsWorld& p_world, EcsEntity p_entity ) : world( p_world ), id( p_entity ) {}

EcsEntityBuilder::~EcsEntityBuilder()
{
    if (!built) {
        NC_LOG_WARN_C( log::ECS, "EcsEntityBuilder discarded without build" );
    }
}

//------------------------------------------------------------------------------

void EcsEntityBuilder::add_component_( const rtti::TypeInfo* type, DynamicArray<uint8_t>&& data )
{
    components.push_back( { type, std::move( data ) } );
}

void EcsEntityBuilder::add_pair_data_(
    const rtti::TypeInfo* first, const rtti::TypeInfo* second, DynamicArray<uint8_t>&& data
)
{
    EcsComponent f_id = world.register_component_type( first );
    EcsComponent s_id = world.register_component_type( second );
    pairs.push_back( { f_id, s_id, first, std::move( data ) } );
}

void EcsEntityBuilder::add_pair_tag_( const rtti::TypeInfo* first, const rtti::TypeInfo* second )
{
    EcsComponent f_id = world.register_component_type( first );
    EcsComponent s_id = world.register_component_type( second );
    pairs.push_back( { f_id, s_id, nullptr, {} } );
}

EcsEntityBuilder& EcsEntityBuilder::add_pair_id( EcsComponent first, EcsComponent second )
{
    pairs.push_back( { first, second, nullptr, {} } );
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
    alias_ = alias;
    return *this;
}

EcsEntityBuilder& EcsEntityBuilder::disabled()
{
    if (!components.empty())
        components.back().disabled = true;
    return *this;
}

//------------------------------------------------------------------------------

EcsEntity EcsEntityBuilder::build()
{
    auto world_ = static_cast<ecs_world_t*>( world.get_native_handle() );

    const char* existing_name = nullptr;
    if (id > 0) {
        // for entity modify op, ecs_entity_init must be provided
        // with both entity name AND entity id. But, in the EcsWorld
        // entity(EcsEntity | const String&) API we only do that
        // exclusively so... we make them whole here
        existing_name = ecs_get_name( world_, id );
    }
    if (existing_name)
        name = existing_name; // gotta do this cus c++ string can't handle nullptr

    // create bare entity
    ecs_entity_desc_t desc{};
    desc.id = id;
    if (!name.empty())
        desc.name = name.c_str();
    if (!alias_.empty())
        desc.symbol = alias_.c_str(); // TODO: rectify this
    ecs_entity_t ent = ecs_entity_init( world_, &desc );
    NC_ASSERT( ent != 0, "Failed to create entity" );

    // set components
    for (auto& c : components) {
        EcsComponent comp_id = world.register_component_type( c.type );
        ecs_set_id( world_, ent, comp_id, c.data.size(), c.data.data() );
        if (c.disabled)
            ecs_enable_id( world_, ent, comp_id, false );
    }

    // add pairs
    for (auto& p : pairs) {
        ecs_id_t pair_id = ecs_make_pair( p.first_id, p.second_id );
        if (p.comp_type) {
            ecs_set_id( world_, ent, pair_id, p.comp_data.size(), p.comp_data.data() );
        } else {
            ecs_add_id( world_, ent, pair_id );
        }
    }

    built = true;
    NC_LOG_DEBUG_C( log::ECS, "Built entity '{}' (id {})", name, ent );
    return static_cast<EcsEntity>( ent );
}

} // namespace nc
