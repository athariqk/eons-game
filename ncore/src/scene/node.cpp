#include <ncore/scene/node.h>

#include <ncore/modules/ecs/ecs_world.h>
#include <ncore/scene/scene.h>

namespace ncore {

Node::Node() {}

Node::~Node() { scene->node_pool.release(this); }

Node *Node::create_child(const std::string &name) {
    auto node = scene->node_pool.acquire();
    node->set_scene(scene);
    node->internal_id = scene->ecs_world.create_entity(name).child_of(internal_id).build();
    NC_LOG_TRACE("created child node '{}' with entity ID {}", name, node->internal_id);
    return node;
}

void Node::destroy_child(Node *child) {
    if (!child || child == this)
        return;
    // TODO: how to make this atomic?
    NC_LOG_TRACE("destroying child node '{}' with entity ID {}", child->get_name(), child->internal_id);
    scene->ecs_world.destroy_entity(child->internal_id);
}

void Node::destroy_children() {}

void Node::reparent_to(Node *child) {}

std::string_view Node::get_name() const { return scene->ecs_world.get_entity_name(internal_id); }

} // namespace ncore
