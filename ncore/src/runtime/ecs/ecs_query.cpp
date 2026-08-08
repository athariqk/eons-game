#include <backends/flecs/flecs_helpers.h>
#include <flecs.h>

#include <ncore/runtime/ecs/ecs_query.h>
#include <ncore/runtime/ecs/ecs_world.h>
#include <ncore/utils/assert.h>
#include <ncore/utils/log.h>

namespace nc {

//------------------------------------------------------------------------------
// EcsQuery::Iterator
//------------------------------------------------------------------------------

EcsIterator::EcsIterator( void* internal ) noexcept
{
    iter_ = internal;
    done_ = false;
}

EcsIterator::EcsIterator( EcsWorld*, void* world, void* query ) :
    world_( world ), query_( query ), kind_( Kind::Query )
{
    auto w = static_cast<ecs_world_t*>( world_ );
    auto q = static_cast<ecs_query_t*>( query_ );

    iter_                              = new ecs_iter_t;
    *static_cast<ecs_iter_t*>( iter_ ) = ecs_query_iter( w, q );

    done_ = !ecs_query_next( static_cast<ecs_iter_t*>( iter_ ) );
}

EcsIterator::~EcsIterator()
{
    auto it = static_cast<ecs_iter_t*>( iter_ );
    if (!done_) {
        ecs_iter_fini( it );
    }
    delete it;
}

EcsIterator::EcsIterator( EcsIterator&& other ) noexcept :
    world_( other.world_ ), query_( other.query_ ), iter_( other.iter_ ), kind_( other.kind_ ), done_( other.done_ )
{
    other.iter_ = nullptr;
    other.done_ = true;
}

EcsIterator& EcsIterator::operator=( EcsIterator&& other ) noexcept
{
    if (this != &other) {
        delete static_cast<ecs_iter_t*>( iter_ );
        world_      = other.world_;
        query_      = other.query_;
        iter_       = other.iter_;
        kind_       = other.kind_;
        done_       = other.done_;
        other.iter_ = nullptr;
        other.done_ = true;
    }
    return *this;
}

bool EcsIterator::operator!=( std::nullptr_t ) const
{
    return !done_;
}

EcsIterator& EcsIterator::operator++()
{
    auto* it = static_cast<ecs_iter_t*>( iter_ );
    done_    = !ecs_query_next( it );
    return *this;
}

double EcsIterator::delta_time() const
{
    auto it = static_cast<ecs_iter_t*>( iter_ );
    return static_cast<double>( it->delta_time );
}

float EcsIterator::delta_time_internal() const
{
    auto it = static_cast<ecs_iter_t*>( iter_ );
    return it->delta_system_time;
}

int32_t EcsIterator::count() const
{
    auto it = static_cast<ecs_iter_t*>( iter_ );
    return static_cast<int32_t>( it->count );
}

EcsEntity EcsIterator::entity( int32_t row ) const
{
    auto it = static_cast<ecs_iter_t*>( iter_ );
    return static_cast<EcsEntity>( it->entities[row] );
}

EcsWorld& EcsIterator::world() const
{
    auto it = static_cast<ecs_iter_t*>( iter_ );
    return *static_cast<EcsWorld*>( ecs_get_binding_ctx( it->world ) );
}

EcsEntity EcsIterator::event()
{
    auto it = static_cast<ecs_iter_t*>( iter_ );
    return it->event;
}

void* EcsIterator::event_payload()
{
    auto it = static_cast<ecs_iter_t*>( iter_ );
    return it->param;
}

void* EcsIterator::user_ctx() const
{
    auto it = static_cast<ecs_iter_t*>( iter_ );
    return it->ctx;
}

//------------------------------------------------------------------------------
// EcsQuery
//------------------------------------------------------------------------------

EcsQuery::EcsQuery( EcsWorld* world_ref, void* world_handle, void* query_handle ) :
    world_ref_( world_ref ), world_( world_handle ), query_( query_handle )
{}

EcsIterator EcsQuery::begin()
{
    return EcsIterator( world_ref_, world_, query_ );
}

std::nullptr_t EcsQuery::end()
{
    return nullptr;
}

//------------------------------------------------------------------------------
// QueryContext
//------------------------------------------------------------------------------

QueryContext::QueryContext( void* iter ) : it_( iter ) {}

double QueryContext::delta_time() const
{
    auto it = static_cast<ecs_iter_t*>( it_ );
    return static_cast<double>( it->delta_time );
}

float QueryContext::delta_time_internal() const
{
    auto it = static_cast<ecs_iter_t*>( it_ );
    return it->delta_system_time;
}

int32_t QueryContext::count() const
{
    auto it = static_cast<ecs_iter_t*>( it_ );
    return static_cast<int32_t>( it->count );
}

EcsEntity QueryContext::entity( int32_t row ) const
{
    auto it = static_cast<ecs_iter_t*>( it_ );
    return static_cast<EcsEntity>( it->entities[row] );
}

EcsWorld& QueryContext::world() const
{
    auto it = static_cast<ecs_iter_t*>( it_ );
    return *static_cast<EcsWorld*>( ecs_get_binding_ctx( it->world ) );
}

EcsEntity QueryContext::event()
{
    auto it = static_cast<ecs_iter_t*>( it_ );
    return it->event;
}

void* QueryContext::event_payload()
{
    auto it = static_cast<ecs_iter_t*>( it_ );
    return it->param;
}

void* QueryContext::user_ctx() const
{
    auto it = static_cast<ecs_iter_t*>( it_ );
    return it->ctx;
}

void* QueryContext::get_component_( int32_t column, size_t size, size_t alignment ) const
{
    ( void ) alignment;
    auto it   = static_cast<ecs_iter_t*>( it_ );
    auto base = ecs_field_w_size( it, size, static_cast<int8_t>( column ) );
    return reinterpret_cast<char*>( base ) + size * current_row_;
}

int32_t QueryContext::resolve_term_index_( const rtti::TypeInfo& info ) const
{
    auto cached = term_cache_.find( &info );
    if (cached != term_cache_.end()) {
        return cached->second;
    }

    auto it              = static_cast<ecs_iter_t*>( it_ );
    auto world           = static_cast<EcsWorld*>( ecs_get_binding_ctx( it->world ) );
    EcsComponent comp_id = world->register_component_type( &info );
    for (int8_t i = 0; i < it->field_count; i++) {
        if (it->ids[i] == static_cast<ecs_id_t>( comp_id )) {
            term_cache_.emplace( &info, i );
            return i;
        }
    }

    String ids_str;
    for (int8_t i = 0; i < it->field_count; i++) {
        if (i)
            ids_str += ", ";
        ids_str += std::to_string( it->ids[i] );
    }
    NC_LOG_TRACE_C( log::ECS, "resolve_term_index_: '{}' MISS — comp_id={} it->ids=[{}]", info.name, comp_id, ids_str );

    return -1;
}

int32_t QueryContext::resolve_pair_index_( const rtti::TypeInfo& first, const rtti::TypeInfo& second ) const
{
    auto key    = std::pair{ &first, &second };
    auto cached = pair_cache_.find( key );
    if (cached != pair_cache_.end()) {
        return cached->second;
    }

    auto it                = static_cast<ecs_iter_t*>( it_ );
    auto world             = static_cast<EcsWorld*>( ecs_get_binding_ctx( it->world ) );
    EcsComponent first_id  = world->register_component_type( &first );
    EcsComponent second_id = world->register_component_type( &second );
    ecs_id_t pair_id       = ecs_make_pair( first_id, second_id );
    for (int8_t i = 0; i < it->field_count; i++) {
        if (it->ids[i] == pair_id) {
            pair_cache_.emplace( key, i );
            return i;
        }
    }

    return -1;
}

//------------------------------------------------------------------------------
// EcsQueryBuilder
//------------------------------------------------------------------------------

struct EcsQueryBuilder::Impl : public detail::FlecsQueryBuilder {
    // inherit constructor so we can instantiate this in EcsQueryBuilder
    using FlecsQueryBuilder::FlecsQueryBuilder;
};

EcsQueryBuilder::EcsQueryBuilder( EcsWorld& world, String name ) :
    pImpl( std::make_unique<Impl>( world, std::move( name ) ) )
{}

EcsQueryBuilder::~EcsQueryBuilder()
{
    if (!pImpl->built) {
        NC_LOG_WARN_C( log::ECS, "EcsQueryBuilder '{}' discarded without build", pImpl->name );
    }
}

void EcsQueryBuilder::add_term_impl( const rtti::TypeInfo* type, uint8_t inout )
{
    NC_ASSERT( type, "Component type not registered in rtti::TypeRegistry" );
    EcsComponent comp_id = pImpl->world.register_component_type( type );

    ecs_term_t term{};
    term.id    = comp_id;
    term.inout = ( inout == 0 ) ? EcsInOutDefault : EcsIn;
    pImpl->terms.push_back( term );
}

void EcsQueryBuilder::add_term_pair_impl(
    const rtti::TypeInfo* first_type, const rtti::TypeInfo* sec_type, uint8_t inout
)
{
    NC_ASSERT( first_type && sec_type, "Pair component types not registered" );
    EcsComponent first_id  = pImpl->world.register_component_type( first_type );
    EcsComponent second_id = pImpl->world.register_component_type( sec_type );

    ecs_term_t term{};
    term.id    = ecs_make_pair( first_id, second_id );
    term.inout = ( inout == 0 ) ? EcsInOutDefault : EcsIn;
    pImpl->terms.push_back( term );
}

EcsQueryBuilder& EcsQueryBuilder::with_pair( EcsEntity first, EcsEntity second )
{
    ecs_term_t term{};
    term.id    = ecs_make_pair( first, second );
    term.inout = EcsInOutDefault;
    pImpl->terms.push_back( term );
    return *this;
}

EcsQueryBuilder& EcsQueryBuilder::all()
{
    ecs_term_t term{};
    term.id    = EcsWildcard;
    term.inout = EcsInOutDefault;
    pImpl->terms.push_back( term );
    return *this;
}

EcsQueryBuilder& EcsQueryBuilder::all_read()
{
    ecs_term_t term{};
    term.id    = EcsWildcard;
    term.inout = EcsIn;
    pImpl->terms.push_back( term );
    return *this;
}

EcsQueryBuilder& EcsQueryBuilder::up()
{
    if (!pImpl->terms.empty()) {
        pImpl->terms.back().src.id |= EcsUp;
        pImpl->terms.back().trav = EcsChildOf;
    }
    return *this;
}

EcsQueryBuilder& EcsQueryBuilder::self()
{
    if (!pImpl->terms.empty()) {
        pImpl->terms.back().src.id |= EcsSelf;
    }
    return *this;
}

EcsQueryBuilder& EcsQueryBuilder::expr( StringView dsl )
{
    pImpl->expr = dsl;
    return *this;
}

const String& EcsQueryBuilder::name() const
{
    return pImpl->name;
}

EcsQuery EcsQueryBuilder::build()
{
    size_t term_count = pImpl->terms.size();
    NC_ASSERT( term_count <= FLECS_TERM_COUNT_MAX, std::format( "Too many query terms ({})", term_count ).c_str() );

    ecs_query_desc_t qdesc = pImpl->get_as_descriptor();
    EcsQuery result        = pImpl->world.create_query_( pImpl->name, &qdesc );
    pImpl->built           = true;
    return result;
}

} // namespace nc
