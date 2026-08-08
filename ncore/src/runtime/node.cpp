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
        scene->get_ecs().entity( p_name ).child_of( p_parent->internal_id ).with<NodeRefComponent>( { this } ).build();
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

void Node::reparent_to( Node* child ) {}

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

Span<EcsComponent> Node::get_components()
{
    auto type = ecs_get_type( reinterpret_cast<ecs_world_t*>( scene->get_ecs().get_native_handle() ), internal_id );
    return { type->array, static_cast<size_t>( type->count ) };
}

StringView Node::get_name() const
{
    return scene->get_ecs().lookup( internal_id );
}

uint64_t Node::get_id() const
{
    return internal_id;
}

//------------------------------------------------------------------------------

void* Node::emplace_component_( const rtti::TypeInfo* type )
{
    return scene->get_ecs().emplace_component_( internal_id, type );
}

void* Node::add_component_( const rtti::TypeInfo* type, const void* data )
{
    scene->get_ecs().set_component_( internal_id, type, data );
    auto obj = scene->get_ecs().get_component_( internal_id, type );
    NC_VERIFY( obj );
    return obj;
}

bool Node::has_component_( const rtti::TypeInfo* type ) const
{
    return scene->get_ecs().has_component_( internal_id, type );
}

void Node::remove_component_( const rtti::TypeInfo* type ) const
{
    scene->get_ecs().remove_component_( internal_id, type );
}

bool Node::has_component( const rtti::TypeInfo* type ) const
{
    return has_component_( type );
}

void Node::remove_component( const rtti::TypeInfo* type )
{
    remove_component_( type );
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
    return Iterator( EcsIterator{}, scene_, true );
}

Node::ChildRange::Iterator::Iterator( EcsIterator iter, Scene* p_scene, bool end ) :
    iter_( std::move( iter ) ), scene_( p_scene ), done_( end )
{
    if (end)
        return;

    if (iter_ != nullptr) {
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
        if (iter_ != nullptr) {
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
