#pragma once

#include <ncore/kernel/errors.h>
#include <ncore/kernel/object.h>

namespace nc {

class ConfFile;

/**
 * @brief IModule is an abstraction of raw engine power as a set of
 * procedural APIs. An instance of it serves as a fundamental building
 * block of the engine, such that, if one instance of it does not exist,
 * then a higher level feature may become impossible to achieve.
 *
 * When creating a new IModule, the line of thinking usually goes like:
 * if I can leverage one or more IModules to achieve what I need, then
 * I probably should just use them. If one or more IModules can't provide
 * what I need, then I may need to create a new one.
 *
 * This is very inspired by Godot's server classes concept.
 * 
 * I think it SHOULD not own state, instead our runtime
 * ECS layer shall be the driver.
 *
 * There is so much alot to unpack regarding architecture and design of this relating
 * to the whole engine. TODO: write more about this so i don't forget
 */
class NCORE_API IModule : public NcObject {
    NCLASS( IModule, NcObject )

public:
    virtual Error init( ConfFile& cfg_file ) = 0;
    virtual void finalize()                  = 0;
};

class NCORE_API NullModule : public IModule {
    NCLASS( NullModule, IModule )

public:
    Error init( ConfFile& cfg_file ) override
    {
        return Error::OK;
    }
    void finalize() override {}
};

} // namespace nc
