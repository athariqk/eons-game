#pragma once

#include <stdint.h>

#include <ncore/kernel/service.h>
#include <ncore/kernel/structures.h>

namespace ncore {

class IWindowService : public IService {
    NCLASS(IWindowService, IService)

public:
    virtual ~IWindowService() = default;

    virtual Vec2 get_resolution() const = 0;
    virtual uint32_t get_window_id() const = 0;
    virtual void set_title(const char *title) const = 0;
};

} // namespace ncore
