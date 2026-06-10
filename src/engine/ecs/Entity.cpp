#include "Entity.h"

#include "World.h"

namespace ncore {

void Entity::add_group(Group p_group) {
    group_bitset[p_group] = true;
    world.add_to_group(this, p_group);
}

} // namespace ncore
