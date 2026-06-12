/* Adapted from: https://github.com/carlbirch/BirchEngine */

#pragma once

namespace ncore {

class Entity;

/**
 * @brief Base class for all components
 * 
 * Components should be pure data structures with minimal logic.
 * All behavior should be implemented in Systems that operate on components.
 */
class Component {
public:
    Entity *entity = nullptr;

    virtual ~Component() = default;

protected:
    Component() = default;
};

} // namespace ncore
