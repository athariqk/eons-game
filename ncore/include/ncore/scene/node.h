#pragma once

#include <ncore/kernel/collection.h>
#include <ncore/kernel/object.h>
#include <ncore/modules/ecs/ecs_entity.h>

namespace ncore {

class EcsWorld;

class NCORE_API Component : public NObject {
    NCLASS( Component, NObject )
};

class NCORE_API Node : public NObject {
    NCLASS( Node, NObject )

public:
    Node();
    ~Node();

    Node( const Node& )            = delete;
    Node& operator=( const Node& ) = delete;

    Node* create_child( const std::string& name = std::string() );
    void destroy_child( Node* child );
    void destroy_children();

    void reparent_to( Node* child );

    void add_component( Component* comp );
    Component* get_component( const rfl::TypeInfo* type ) const;
    bool has_component( const rfl::TypeInfo* type ) const;

    std::string_view get_name() const;

private:
    friend class Scene;

    Scene* scene = nullptr;
    void set_scene( Scene* s )
    {
        scene = s;
    }

    Node* parent            = nullptr;
    EcsEntityId internal_id = INVALID_ENTITY_ID;

    // minimum of 32 managed components sounds good
    PagedObjectPool<Component> components{ 32 };
};

} // namespace ncore
