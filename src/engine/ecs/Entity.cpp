#include "Entity.h"

#include "World.h"

namespace Aeon {

void Entity::AddGroup(Group mGroup) {
    m_groupBitset[mGroup] = true;
    m_world.AddToGroup(this, mGroup);
}

} // namespace Aeon
