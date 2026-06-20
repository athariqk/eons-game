#pragma once

#include <ncore/kernel/object.h>

namespace ncore {

/**
 * @brief The game world for an application.
 *
 * Derived classes should implement the specific behavior of the game world.
 */
class IGameWorld : public NObject {
    NCLASS(IGameWorld, NObject)

public:
    virtual ~IGameWorld() = default;

    // Lifecycle hooks
    virtual void on_init() = 0;
    virtual void on_fixed_update(double p_delta, uint64_t ticks) = 0;
    virtual void on_variable_update(double p_delta) = 0;
    virtual void on_post_update(double p_delta) = 0;
    virtual void on_finish() = 0;
};

} // namespace ncore
