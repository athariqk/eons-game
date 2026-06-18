#pragma once

#include "object.h"

namespace ncore {

class IWorld : public NcObject {
    NCLASS(IWorld, NcObject)

public:
    virtual ~IWorld() = default;

    // Lifecycle hooks (called by MainLoop)
    virtual void on_init() = 0;
    virtual void on_fixed_update(double p_delta, uint64_t ticks) = 0;
    virtual void on_variable_update(double p_delta) = 0;
    virtual void on_post_update(double p_delta) = 0;
    virtual void on_finish() = 0;
};

} // namespace ncore
