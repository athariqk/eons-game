#include <algorithm>
#include <vector>

#include <flecs.h>
#include <flecs/addons/pipeline.h>

#include <ncore/kernel/collection.h>
#include <ncore/kernel/memory.h>
#include <ncore/runtime/ecs_entity.h>
#include <ncore/runtime/ecs_world.h>
#include <ncore/utils/log.h>

namespace nc {

static void flecs_log_callback( int32_t level, const char* file, int32_t line, const char* msg )
{
    int mapped_lvl = 1; // debug
    switch (level) {
        case 0:
            mapped_lvl = 0; // trace
            break;
        case -2:
            mapped_lvl = 3; // warn
            break;
        case -3:
            mapped_lvl = 4; // error
            break;
        case -4:
            mapped_lvl = 5; // fatal
            break;
    }
    log::log_message( log::ECS, mapped_lvl, file, nullptr, line, msg );
}

// static function so this is called only once
static void init_flecs_os_api()
{
    ecs_os_set_api_defaults();
    ecs_os_api_t os_api = ecs_os_get_api();
    os_api.malloc_      = []( ecs_size_t size ) -> void* { return memalloc( static_cast<size_t>( size ) ); };
    os_api.free_        = []( void* ptr ) { memfree( ptr ); };
    os_api.realloc_     = []( void* ptr, ecs_size_t size ) -> void* {
        return memrealloc( ptr, static_cast<size_t>( size ) );
    };
    os_api.calloc_ = []( ecs_size_t size ) -> void* { return memcalloc( 1, static_cast<size_t>( size ) ); };
    os_api.log_    = flecs_log_callback;
    ecs_os_set_api( &os_api );
}

//------------------------------------------------------------------------------

struct EcsWorld::Impl {
    ecs_world_t* world = nullptr;
    UnorderedMap<const rtti::TypeInfo*, EcsComponentId> comp_id_map;
    UnorderedMap<std::string, ecs_query_t*>
        query_cache; // TODO: is this even necessary? figure out how flecs cache queries

#if defined( NC_DEBUG )
    ecs_http_server_t* http_svr;
#endif
};

EcsWorld::EcsWorld( IGameWorld& p_game_world ) : pImpl( std::make_unique<Impl>() ), g_world( p_game_world )
{
    static bool os_api_init = ( init_flecs_os_api(), true );
    ( void ) os_api_init;

    pImpl->world = ecs_init();
    ecs_set_binding_ctx( pImpl->world, this, nullptr );
    ecs_log_set_level( 0 );

#if defined( NC_DEBUG )
    ecs_http_server_desc_t http_desc{};
    http_desc.port  = ECS_REST_DEFAULT_PORT;
    pImpl->http_svr = ecs_rest_server_init( pImpl->world, &http_desc );
    ecs_http_server_start( pImpl->http_svr );
#endif
}

EcsWorld::~EcsWorld()
{
    ecs_fini( pImpl->world );

#if defined( NC_DEBUG )
    ecs_http_server_stop( pImpl->http_svr );
    ecs_rest_server_fini( pImpl->http_svr );
#endif
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

static void handle_ctor_hook( void* ptr, int32_t count, const ecs_type_info_t* type_info )
{
    auto ctx       = reinterpret_cast<rtti::RecordInfo*>( type_info->hooks.lifecycle_ctx );
    char* byte_ptr = static_cast<char*>( ptr );
    for (int i = 0; i < count; i++) {
        void* item_ptr = byte_ptr + ( i * type_info->size );
        ctx->construct( item_ptr );
        ecs_trace( "%s: ctor called at %p", type_info->name, item_ptr );
    }
}

static void handle_dtor_hook( void* ptr, int32_t count, const ecs_type_info_t* type_info )
{
    auto ctx       = reinterpret_cast<rtti::RecordInfo*>( type_info->hooks.lifecycle_ctx );
    char* byte_ptr = static_cast<char*>( ptr );
    for (int i = 0; i < count; i++) {
        void* item_ptr = byte_ptr + ( i * type_info->size );
        ctx->destruct( item_ptr );
        ecs_trace( "%s: dtor called at %p", type_info->name, item_ptr );
    }
}

static void handle_copy_hook( void* dst_ptr, const void* src_ptr, int32_t count, const ecs_type_info_t* type_info )
{
    auto ctx            = reinterpret_cast<rtti::RecordInfo*>( type_info->hooks.lifecycle_ctx );
    const char* src_arr = static_cast<const char*>( src_ptr );
    char* dst_arr       = static_cast<char*>( dst_ptr );
    for (int i = 0; i < count; i++) {
        const void* src_item_ptr = src_arr + ( i * type_info->size );
        void* dst_item_ptr       = dst_arr + ( i * type_info->size );
        ctx->clone( src_item_ptr, dst_item_ptr );
        ecs_trace( "%s: copy assignment called - from %p to %p", type_info->name, src_item_ptr, dst_item_ptr );
    }
}

static void handle_move_hook( void* dst_ptr, void* src_ptr, int32_t count, const ecs_type_info_t* type_info )
{
    auto ctx            = reinterpret_cast<rtti::RecordInfo*>( type_info->hooks.lifecycle_ctx );
    const char* src_arr = static_cast<const char*>( src_ptr );
    char* dst_arr       = static_cast<char*>( dst_ptr );
    for (int i = 0; i < count; i++) {
        const void* src_item_ptr = src_arr + ( i * type_info->size );
        void* dst_item_ptr       = dst_arr + ( i * type_info->size );
        ctx->replace( src_item_ptr, dst_item_ptr );
        ecs_trace( "%s: move assignment called - from %p to %p", type_info->name, src_item_ptr, dst_item_ptr );
    }
}

EcsEntityId EcsWorld::register_component_type( const rtti::TypeInfo* type )
{
    auto it = pImpl->comp_id_map.find( type );
    if (it != pImpl->comp_id_map.end())
        return static_cast<EcsComponentId>( it->second ); // returns existing

    // auto-register

    ecs_type_hooks_t hooks{};
    if (type->is_record()) {
        hooks.ctor = handle_ctor_hook;
        hooks.dtor = handle_dtor_hook;
        hooks.copy = handle_copy_hook;
        hooks.move = handle_move_hook;
        // any other lifecycle hooks is illegal for now
        hooks.lifecycle_ctx = const_cast<void*>( reinterpret_cast<const void*>( type ) );
    }

    ecs_type_info_t type_info{
        .size      = static_cast<ecs_size_t>( type->size ),
        .alignment = static_cast<ecs_size_t>( type->alignment ),
        .hooks     = hooks,
        .component = {},
        .name      = type->name
    };

    ecs_component_desc_t desc{};
    desc._canary = 0;
    desc.entity  = create_entity_impl_( type->name );
    desc.type    = type_info;

    auto comp_id = ecs_component_init( pImpl->world, &desc );
    NC_ASSERT( comp_id != 0, std::format( "Failed to auto-register component '{}'", type->name ).c_str() );

    pImpl->comp_id_map[type] = comp_id;

    return static_cast<EcsEntityId>( comp_id );
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

EcsObserverBuilder EcsWorld::create_observer( std::string_view name )
{
    return EcsObserverBuilder( *this, std::string( name ) );
}

void EcsWorld::finalize_ordering()
{
    auto* world             = pImpl->world;
    ecs_entity_t order_comp = ecs_lookup( world, "nc::SystemOrder" );
    if (!order_comp) {
        return;
    }

    ecs_entity_t phases[] = {
        EcsOnStart, EcsPreFrame, EcsPreUpdate, EcsOnUpdate, EcsPostUpdate, EcsPostFrame,
    };

    for (ecs_entity_t phase : phases) {
        ecs_query_desc_t qdesc{};
        qdesc.terms[0].id    = ecs_pair( EcsDependsOn, phase );
        qdesc.terms[1].id    = order_comp;
        qdesc.terms[1].inout = EcsIn;

        ecs_query_t* q = ecs_query_init( world, &qdesc );
        if (!q) {
            continue;
        }

        std::vector<std::pair<ecs_entity_t, int32_t>> systems;
        ecs_iter_t it = ecs_query_iter( world, q );
        while (ecs_query_next( &it )) {
            for (int i = 0; i < it.count; i++) {
                const auto* order = static_cast<const int32_t*>( ecs_get_id( world, it.entities[i], order_comp ) );
                if (order) {
                    systems.emplace_back( it.entities[i], *order );
                }
            }
        }
        ecs_query_fini( q );

        std::stable_sort( systems.begin(), systems.end(), []( const auto& a, const auto& b ) {
            return a.second < b.second;
        } );

        for (size_t i = 1; i < systems.size(); i++) {
            ecs_add_pair( world, systems[i].first, EcsDependsOn, systems[i - 1].first );
        }
    }
}

//------------------------------------------------------------------------------

void* EcsWorld::get_native_handle() const
{
    return pImpl->world;
}

EcsEntityId EcsWorld::create_entity_impl_( const std::string& name )
{
    ecs_entity_desc_t desc{};
    if (!name.empty())
        desc.name = name.c_str(); // this is fine, flecs will do strdup
    ecs_entity_t result = ecs_entity_init( pImpl->world, &desc );
    NC_ASSERT( result != 0, "Failed to create entity" );
    return static_cast<EcsEntityId>( result );
}

EcsComponentId EcsWorld::set_component_( EcsEntityId eid, const rtti::TypeInfo* type, const void* data )
{
    // auto ent_name = ecs_get_name( pImpl->world, eid );
    // ecs_trace(
    //     "setting component value of %s to entity (name: %s, ID %d)", type->name, ent_name ? ent_name : "n/a", eid
    //);

    EcsComponentId registered_comp_id = register_component_type( type );

    // IF entity ID is invalid, we can use the component ID as entity ID,
    // meaning it is a singleton entity
    if (eid == INVALID_ENTITY_ID) {
        eid = registered_comp_id;
    }

    if (eid == registered_comp_id) { // is singleton?
        ecs_add_id( pImpl->world, eid, EcsSingleton );
    }

    ecs_set_id( pImpl->world, eid, registered_comp_id, type->size, data );

    return registered_comp_id;
}

EcsComponentId EcsWorld::emplace_component_( EcsEntityId eid, const rtti::TypeInfo* type )
{
    NC_ASSERT_RETVAL( type->is_record(), INVALID_ENTITY_ID, "Emplacing a non-record component is not possible" );

    EcsComponentId registered_comp_id = register_component_type( type );
    if (eid == INVALID_ENTITY_ID) {
        eid = registered_comp_id;
    }

    if (eid == registered_comp_id) { // is singleton?
        ecs_add_id( pImpl->world, eid, EcsSingleton );
    }

    bool is_new = false;
    auto ptr    = ecs_emplace_id( pImpl->world, eid, registered_comp_id, type->size, &is_new );
    if (is_new) {
        static_cast<const rtti::RecordInfo*>( type )->construct( ptr );
    }

    return registered_comp_id;
}

void* EcsWorld::get_component_( EcsEntityId id, const rtti::TypeInfo* type ) const
{
    auto it = pImpl->comp_id_map.find( type );
    if (it == pImpl->comp_id_map.end())
        return nullptr;
    if (id == INVALID_ENTITY_ID) { // is singleton?
        id = it->second;
    }
    return ecs_get_mut_id( pImpl->world, id, it->second );
}

const void* EcsWorld::get_component_ro_( EcsEntityId id, const rtti::TypeInfo* type ) const
{
    auto it = pImpl->comp_id_map.find( type );
    if (it == pImpl->comp_id_map.end())
        return nullptr;
    if (id == INVALID_ENTITY_ID) { // is singleton?
        id = it->second;
    }
    return ecs_get_id( pImpl->world, id, it->second );
}

bool EcsWorld::has_component_( EcsEntityId id, const rtti::TypeInfo* type ) const
{
    auto it = pImpl->comp_id_map.find( type );
    if (it == pImpl->comp_id_map.end())
        return false;
    if (id == INVALID_ENTITY_ID) { // is singleton?
        id = it->second;
    }
    return ecs_has_id( pImpl->world, id, it->second ) != 0;
}

void EcsWorld::remove_component_( EcsEntityId id, const rtti::TypeInfo* type ) const
{
    auto it = pImpl->comp_id_map.find( type );
    if (it == pImpl->comp_id_map.end())
        return;
    if (id == INVALID_ENTITY_ID) { // is singleton?
        id = it->second;
    }
    ecs_remove_id( pImpl->world, id, it->second );
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
