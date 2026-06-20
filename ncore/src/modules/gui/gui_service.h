#pragma once
#include <ncore/kernel/service.h>

namespace ncore {

/**
 * @brief Immediate-mode GUI implementation
 */
class IGuiService : public IService {
    NCLASS(IGuiService, IService)

public:
    virtual ~IGuiService() = default;

    virtual void begin() {}
    virtual void end() {}
};

} // namespace ncore
