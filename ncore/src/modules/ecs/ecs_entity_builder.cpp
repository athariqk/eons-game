#include <flecs.h>

#include <ncore/modules/ecs/ecs_entity.h>
#include <ncore/modules/ecs/ecs_world.h>
#include <ncore/utils/log.h>

namespace ncore {

struct EcsEntityBuilder::Impl {
    struct ComponentEntry {
        const rfl::TypeInfo* type;
        std::vector<uint8_t> data;
    };
    struct PairEntry {
        EcsComponentId first_id;
        EcsComponentId second_id;
        const rfl::TypeInfo* comp_type = nullptr; // nullptr -> tag-only
        std::vector<uint8_t> comp_data;
    };

    EcsWorld& world;
    std::string name;
    std::string alias_; // TODO: could be a node path :)
    std::vector<ComponentEntry> components;
    std::vector<PairEntry> pairs;
    bool built = false;
};

//------------------------------------------------------------------------------

EcsEntityBuilder::EcsEntityBuilder( EcsWorld& world, const std::string& name ) :
    pImpl( std::make_unique<Impl>( world, name ) )
{}

EcsEntityBuilder::~EcsEntityBuilder()
{
    if (!pImpl->built) {
        NC_LOG_WARN_C( log::ECS, "EcsEntityBuilder discarded without build" );
    }
}

//------------------------------------------------------------------------------

void EcsEntityBuilder::add_component_( const rfl::TypeInfo* type, std::vector<uint8_t>&& data )
{
    pImpl->components.push_back( { type, std::move( data ) } );
}

void EcsEntityBuilder::add_pair_data_(
    const rfl::TypeInfo* first, const rfl::TypeInfo* second, std::vector<uint8_t>&& data
)
{
    EcsComponentId f_id = pImpl->world.register_component_type( first );
    EcsComponentId s_id = pImpl->world.register_component_type( second );
    pImpl->pairs.push_back( { f_id, s_id, first, std::move( data ) } );
}

void EcsEntityBuilder::add_pair_tag_( const rfl::TypeInfo* first, const rfl::TypeInfo* second )
{
    EcsComponentId f_id = pImpl->world.register_component_type( first );
    EcsComponentId s_id = pImpl->world.register_component_type( second );
    pImpl->pairs.push_back( { f_id, s_id, nullptr, {} } );
}

EcsEntityBuilder& EcsEntityBuilder::add_pair_id( EcsComponentId first, EcsComponentId second )
{
    pImpl->pairs.push_back( { first, second, nullptr, {} } );
    return *this;
}

EcsEntityBuilder& EcsEntityBuilder::child_of( EcsEntityId parent )
{
    return add_pair_id( static_cast<EcsComponentId>( EcsChildOf ), parent );
}

EcsEntityBuilder& EcsEntityBuilder::is_a( EcsEntityId base )
{
    return add_pair_id( static_cast<EcsComponentId>( EcsIsA ), base );
}

EcsEntityBuilder& EcsEntityBuilder::depends_on( EcsEntityId target )
{
    return add_pair_id( static_cast<EcsComponentId>( EcsDependsOn ), target );
}

EcsEntityBuilder& EcsEntityBuilder::alias( std::string_view alias )
{
    pImpl->alias_ = alias;
    return *this;
}

//------------------------------------------------------------------------------

EcsEntityId EcsEntityBuilder::build()
{
    auto* world = static_cast<ecs_world_t*>( pImpl->world.get_native_handle_() );

    // create bare entity
    ecs_entity_desc_t desc{};
    if (!pImpl->name.empty())
        desc.name = pImpl->name.c_str();
    if (!pImpl->alias_.empty())
        desc.symbol = pImpl->alias_.c_str(); // TODO: rectify this
    ecs_entity_t ent = ecs_entity_init( world, &desc );
    NC_ASSERT( ent != 0, "failed to create entity" );

    // set components
    for (auto& c : pImpl->components) {
        EcsComponentId comp_id = pImpl->world.register_component_type( c.type );
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
    NC_LOG_TRACE_C( log::ECS, "built entity '{}' (id {})", pImpl->name, ent );
    return static_cast<EcsEntityId>( ent );
}

} // namespace ncore
