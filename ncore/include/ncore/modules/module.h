#pragma once

#include <ncore/kernel/errors.h>
#include <ncore/kernel/object.h>

namespace nc {

/**
 * @brief IModule is an abstraction of raw engine power.
 * I think it SHOULD not own state, instead our runtime ECS layer shall
 * be the driver.
 *
 * There is so much alot to unpack regarding architecture and design of this relating
 * to the whole engine. TODO: write more about this so i don't forget
 */
class NCORE_API IModule : public NcObject {
    NCLASS( IModule, NcObject )

public:
    virtual Error init()    = 0;
    virtual void finalize() = 0;
};

class NCORE_API NullModule : public IModule {
    NCLASS( NullModule, IModule )

public:
    Error init() override
    {
        return Error::OK;
    }
    void finalize() override {}
};

} // namespace nc
