#include <flecs.h>

#include <ncore/runtime/ecs/ecs_world.h>
#include <ncore/runtime/node.h>
#include <ncore/runtime/scene.h>

namespace nc {

Node::Node( const String& p_name, Scene* p_scene, Node* p_parent ) : scene( p_scene ), parent( p_parent )
{
    if (!p_parent) {
        p_parent = scene->root();
    }
    NC_VERIFY( p_parent );

    internal_id =
        scene->get_ecs().entity( p_name ).child_of( p_parent->internal_id ).add<NodeRefComponent>( { this } ).build();
    emit_event<NodeAddedEvent>( internal_id );
}

Node::~Node() {}

bool Node::operator==( const Node& other ) const
{
    return internal_id == other.internal_id;
}

bool Node::operator==( const Node* other ) const
{
    return other && other->internal_id == internal_id;
}

bool Node::operator!=( const Node& other ) const
{
    return internal_id != other.internal_id;
}

void Node::reparent_to( Node* p_parent )
{
    NC_FAIL_MSG_RET(
        !p_parent->is_descendant_of( *this ),
        std::format( "Trying to reparent node `{}` to its descendant `{}`", get_name(), p_parent->get_name() ).c_str()
    );

    auto world = reinterpret_cast<ecs_world_t*>( scene->get_ecs().get_native_handle() );
    if (parent)
        ecs_remove_pair( world, internal_id, EcsChildOf, parent->internal_id );
    ecs_add_pair( world, internal_id, EcsChildOf, p_parent->internal_id );
    parent = p_parent;
}

bool Node::is_descendant_of( Node& node )
{
    for (auto& child : node.get_children()) {
        if (child == *this)
            return true;
        if (is_descendant_of( child ))
            return true;
    }
    return false;
}

void Node::destroy()
{
    scene->queue_destroy_node( this );
}

Node* Node::create_child( const String& name )
{
    auto node       = node_pool->acquire( name, scene, this );
    node->node_pool = node_pool;
    NC_LOG_TRACE( "Created child node '{}' with entity ID {}", name, node->internal_id );
    return node;
}

Node::ChildRange Node::get_children()
{
    if (!child_query.is_valid()) {
        auto name   = std::format( "{}_{}_ChildQuery", get_name(), internal_id );
        child_query = scene->get_ecs()
                          .query( name )
                          .with<NodeRefComponent>()
                          .with_pair( static_cast<EcsEntity>( EcsChildOf ), internal_id )
                          .build();
    }

    return Node::ChildRange( child_query, scene );
}

uint32_t Node::get_child_count()
{
    uint32_t count = 0;
    for (auto& child : get_children()) {
        ( void ) child;
        ++count;
    }
    return count;
}

void Node::destroy_children()
{
    for (auto& child : get_children()) {
        child.destroy();
    }
}

const void* Node::get_component_const( const rtti::TypeInfo* type ) const
{
    return scene->get_ecs().get_component_const_( internal_id, type );
}

void* Node::get_component( const rtti::TypeInfo* type ) const
{
    return scene->get_ecs().get_component_( internal_id, type );
}

Span<Component> Node::get_components()
{
    return { components.data(), component_count };
}

void* Node::add_component( const rtti::TypeInfo* type )
{
    if (has_component_( type ))
        return get_component( type );

    void* result = scene->get_ecs().add_component_( internal_id, type );
    auto comp_id = scene->get_ecs().register_component_type( type );
    track_ecs_component( type, comp_id );
    return result;
}

void* Node::add_component( const rtti::TypeInfo* type, const void* data )
{
    if (has_component_( type ))
        return get_component( type );

    auto comp_id = scene->get_ecs().set_component_( internal_id, type, data );
    track_ecs_component( type, comp_id );
    auto obj = get_component( type );
    NC_VERIFY( obj );
    return obj;
}

StringView Node::get_name() const
{
    return scene->get_ecs().lookup( internal_id );
}

void Node::set_name( StringView name )
{
    auto world = reinterpret_cast<ecs_world_t*>( scene->get_ecs().get_native_handle() );
    ecs_set_name( world, internal_id, name.data() );
}

uint64_t Node::get_id() const
{
    return internal_id;
}

bool* Node::get_active()
{
    return &active;
}

//------------------------------------------------------------------------------

void Node::track_ecs_component( const rtti::TypeInfo* type, EcsComponent id )
{
    auto world                    = reinterpret_cast<ecs_world_t*>( scene->get_ecs().get_native_handle() );
    components[component_count++] = Component{ id, true, ecs_has_id( world, id, EcsCanToggle ) };
}

bool Node::has_component_( const rtti::TypeInfo* type ) const
{
    return scene->get_ecs().has_component_( internal_id, type );
}

void Node::remove_component_( const rtti::TypeInfo* type )
{
    scene->get_ecs().remove_component_( internal_id, type );
    auto comp_id = scene->get_ecs().register_component_type( type );
    for (uint32_t i = 0; i < component_count; ++i) {
        if (components[i].EcsId == comp_id) {
            components[i] = components[component_count - 1];
            --component_count;
            break;
        }
    }
}

bool Node::has_component( const rtti::TypeInfo* type ) const
{
    return has_component_( type );
}

void Node::remove_component( const rtti::TypeInfo* type )
{
    remove_component_( type );
}

void Node::set_component_enabled( const rtti::TypeInfo* type, bool enabled )
{
    auto comp_id = scene->get_ecs().register_component_type( type );
    auto world   = reinterpret_cast<ecs_world_t*>( scene->get_ecs().get_native_handle() );
    ecs_enable_id( world, internal_id, comp_id, enabled );
}

bool Node::is_component_enabled( const rtti::TypeInfo* type ) const
{
    auto comp_id = scene->get_ecs().register_component_type( type );
    auto world   = reinterpret_cast<ecs_world_t*>( scene->get_ecs().get_native_handle() );
    return ecs_is_enabled_id( world, internal_id, comp_id );
}

void Node::mark_component_modified( const rtti::TypeInfo* type ) const
{
    auto comp_id = scene->get_ecs().register_component_type( type );
    auto world   = reinterpret_cast<ecs_world_t*>( scene->get_ecs().get_native_handle() );
    ecs_modified_id( world, internal_id, comp_id );
}

void Node::emit_event_( const rtti::TypeInfo* type, EcsEntity target, const void* data ) const
{
    scene->get_ecs().emit_event_( type, target, data );
}

//------------------------------------------------------------------------------

Node::ChildRange::ChildRange( EcsQuery& p_query, Scene* p_scene ) : query( p_query ), scene_( p_scene ) {}

Node::ChildRange::ChildRange( Node::ChildRange&& o ) noexcept : query( o.query ), scene_( o.scene_ ) {}

Node::ChildRange::Iterator Node::ChildRange::begin()
{
    return Iterator( query.begin(), scene_, false );
}

Node::ChildRange::Iterator Node::ChildRange::end()
{
    return Iterator( EcsTableIterator{}, scene_, true );
}

Node::ChildRange::Iterator::Iterator( EcsTableIterator iter, Scene* p_scene, bool end ) :
    iter_( std::move( iter ) ), scene_( p_scene ), done_( end )
{
    if (end)
        return;

    if (!iter_.is_done()) {
        count_ = iter_.count();
        index_ = 0;
        if (count_ == 0) {
            done_ = true;
        }
    } else {
        done_ = true;
    }
}

Node::ChildRange::Iterator::reference Node::ChildRange::Iterator::operator*()
{
    auto it     = static_cast<ecs_iter_t*>( iter_.get_internal_iter() );
    auto entity = it->entities[index_];
    auto comp   = scene_->get_ecs().resolve_component<NodeRefComponent>();
    auto ref    = static_cast<const NodeRefComponent*>( ecs_get_id( it->world, entity, comp ) );
    NC_ASSERT( ref && ref->node, "Child entity missing NodeRefComponent" );
    return *ref->node;
}

Node::ChildRange::Iterator& Node::ChildRange::Iterator::operator++()
{
    ++index_;
    if (index_ >= count_) {
        ++iter_;
        if (!iter_.is_done()) {
            count_ = iter_.count();
            index_ = 0;
        } else {
            done_ = true;
        }
    }
    return *this;
}

bool Node::ChildRange::Iterator::operator!=( const Iterator& o ) const
{
    return done_ != o.done_;
}

} // namespace nc
