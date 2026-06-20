#pragma once

#include <ncore/kernel/object.h>

namespace ncore {

/**
 * @brief Service is an abstraction of raw engine power.
 * I think it SHOULD not own state, instead our runtime ECS layer shall
 * be the driver.
 *
 * There is so much alot to unpack regarding architecture and design of this relating
 * to the whole engine. TODO: write more about this so i don't forget
 */
class IService : public NObject {
    NCLASS(IService, NObject)

public:
    virtual ~IService() = default;

    virtual Error init() = 0;
    virtual void cleanup() = 0;
};

class NullService : public IService {
    NCLASS(NullService, IService)

public:
    Error init() override { return Error::OK; }
    void cleanup() override {}
};

} // namespace ncore
