#include <flecs.h>
#include <flecs/addons/pipeline.h>
#include <flecs/addons/system.h>

#include <ncore/kernel/memory.h>
#include <ncore/runtime/ecs_entity.h>
#include <ncore/runtime/ecs_world.h>
#include <ncore/utils/log.h>

namespace nc {

static void init_flecs_os_allocator()
{
    ecs_os_set_api_defaults();
    ecs_os_api_t os_api = ecs_os_get_api();
    os_api.malloc_      = []( ecs_size_t size ) -> void* { return memalloc( static_cast<size_t>( size ) ); };
    os_api.free_        = []( void* ptr ) { memfree( ptr ); };
    os_api.realloc_     = []( void* ptr, ecs_size_t size ) -> void* {
        return memrealloc( ptr, static_cast<size_t>( size ) );
    };
    os_api.calloc_ = []( ecs_size_t size ) -> void* { return memcalloc( 1, static_cast<size_t>( size ) ); };
    ecs_os_set_api( &os_api );
}

//------------------------------------------------------------------------------

struct EcsWorld::Impl {
    ecs_world_t* world = nullptr;
    std::unordered_map<const rtti::TypeInfo*, EcsComponentId> comp_id_map;
    std::unordered_map<std::string, ecs_query_t*>
        query_cache; // TODO: is this even necessary? figure out how flecs cache queries
};

EcsWorld::EcsWorld( ModuleRegistry& p_modules ) : pImpl( std::make_unique<Impl>() ), modules( p_modules )
{
    static bool os_allocator_init = ( init_flecs_os_allocator(), true );
    ( void ) os_allocator_init;

    pImpl->world = ecs_init();
    ecs_set_binding_ctx( pImpl->world, this, nullptr );
}

EcsWorld::~EcsWorld()
{
    ecs_fini( pImpl->world );
}

//------------------------------------------------------------------------------

void EcsWorld::progress( double delta_time )
{
    ecs_progress( pImpl->world, static_cast<float>( delta_time ) );
}

//------------------------------------------------------------------------------

EcsEntityBuilder EcsWorld::create_entity( const std::string& name )
{
    return EcsEntityBuilder( *this, name );
}

EcsEntityId EcsWorld::create_entity_impl_( const std::string& name )
{
    ecs_entity_desc_t desc{};
    if (!name.empty())
        desc.name = name.c_str();
    ecs_entity_t result = ecs_entity_init( pImpl->world, &desc );
    NC_ASSERT( result != 0, "Failed to create entity" );
    return static_cast<EcsEntityId>( result );
}

EcsEntityId EcsWorld::get_entity( std::string_view name, EcsEntityId parent ) const
{
    auto ent = INVALID_ENTITY_ID;
    if (parent != INVALID_ENTITY_ID) {
        ent = ecs_lookup_child( pImpl->world, parent, name.data() );
    } else {
        ent = ecs_lookup( pImpl->world, name.data() );
    }
    return static_cast<EcsEntityId>( ent );
}

std::string_view EcsWorld::get_entity_name( EcsEntityId entity ) const
{
    return std::string_view( ecs_get_name( pImpl->world, entity ) );
}

std::span<EcsEntityId> EcsWorld::get_entities() const
{
    ecs_entities_t ents = ecs_get_entities( pImpl->world );
    return std::span<EcsEntityId>( const_cast<EcsEntityId*>( ents.ids ), static_cast<size_t>( ents.alive_count ) );
}

size_t EcsWorld::get_entity_count( bool alive ) const
{
    ecs_entities_t ents = ecs_get_entities( pImpl->world );
    return static_cast<size_t>( alive ? ents.alive_count : ents.count );
}

void EcsWorld::destroy_entity( EcsEntityId entity )
{
    ecs_delete( pImpl->world, entity );
    NC_LOG_TRACE_C( log::ECS, "Destroyed entity: {}", entity );
}

//------------------------------------------------------------------------------

EcsEntityId EcsWorld::set_component_( EcsEntityId eid, const rtti::TypeInfo* type, const void* data, size_t sz )
{
    // IF entity ID is invalid, we can use the component ID as entity ID,
    // meaning it is a singleton entity
    NC_LOG_TRACE_C( log::ECS, "Binding data of size {} to entity {}", sz, eid );
    EcsComponentId comp_id = register_component_type( type );
    if (eid == INVALID_ENTITY_ID) {
        eid = comp_id; // singleton
    }
    ecs_set_id( pImpl->world, eid, comp_id, sz, const_cast<void*>( data ) );
    return eid;
}

void* EcsWorld::get_component_( EcsComponentId eid, const rtti::TypeInfo* type ) const
{
    auto it = pImpl->comp_id_map.find( type );
    if (it == pImpl->comp_id_map.end())
        return nullptr;
    return const_cast<void*>( ecs_get_id( pImpl->world, eid, it->second ) );
}

bool EcsWorld::has_component_( EcsComponentId eid, const rtti::TypeInfo* type ) const
{
    auto it = pImpl->comp_id_map.find( type );
    if (it == pImpl->comp_id_map.end())
        return false;
    return ecs_has_id( pImpl->world, eid, it->second ) != 0;
}

//------------------------------------------------------------------------------

void EcsWorld::add_pair( EcsEntityId entity, EcsComponentId first, EcsComponentId second )
{
    ecs_add_pair( pImpl->world, entity, first, second );
}

bool EcsWorld::has_pair( EcsEntityId entity, EcsComponentId first, EcsComponentId second ) const
{
    return ecs_has_pair( pImpl->world, entity, first, second );
}

void EcsWorld::remove_pair( EcsEntityId entity, EcsComponentId first, EcsComponentId second )
{
    ecs_remove_pair( pImpl->world, entity, first, second );
}

//------------------------------------------------------------------------------

EcsSystemBuilder EcsWorld::create_system( std::string_view name )
{
    return EcsSystemBuilder( *this, std::string( name ) );
}

EcsQueryBuilder EcsWorld::create_query( std::string_view name )
{
    return EcsQueryBuilder( *this, std::string( name ) );
}

EcsComponentId EcsWorld::register_component_type( const rtti::TypeInfo* type )
{
    auto it = pImpl->comp_id_map.find( type );
    if (it != pImpl->comp_id_map.end())
        return static_cast<EcsComponentId>( it->second ); // returns existing

    // auto-register
    ecs_type_info_t type_info{
        .size      = static_cast<ecs_size_t>( type->size ),
        .alignment = static_cast<ecs_size_t>( type->alignment ),
        .hooks     = {},
        .component = {},
        .name      = type->name
    };
    ecs_component_desc_t desc{ ._canary = 0, .entity = 0, .type = type_info };
    auto comp_id = ecs_component_init( pImpl->world, &desc );
    NC_ASSERT( comp_id != 0, std::format( "Failed to auto-register component '{}'", type->name ).c_str() );
    pImpl->comp_id_map[type] = comp_id;
    return static_cast<EcsComponentId>( comp_id );
}

void* EcsWorld::get_native_handle_() const
{
    return pImpl->world;
}

EcsQuery EcsWorld::create_query_( const std::string& name, void* data )
{
    auto* qdesc = static_cast<ecs_query_desc_t*>( data );

    auto& cached = pImpl->query_cache[name];
    if (cached) {
        NC_LOG_TRACE_C( log::ECS, "Reusing cached query '{}'", name );
        return EcsQuery( this, pImpl->world, cached );
    }

    ecs_query_t* q = ecs_query_init( pImpl->world, qdesc );
    NC_ASSERT( q, std::format( "Failed to create query '{}'", name ).c_str() );
    cached = q;
    NC_LOG_TRACE_C( log::ECS, "Created query '{}'", name );
    return EcsQuery( this, pImpl->world, q );
}

} // namespace nc
