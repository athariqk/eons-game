#pragma once

#include <stdint.h>

#include <ncore/kernel/structures.h>
#include <ncore/modules/service.h>
#include <ncore/modules/video/viewport.h>

namespace ncore {

class Viewport;

/**
 * @brief IWindowService defines an interface for OS window management.
 */
class IWindowService : public IService {
    NCLASS(IWindowService, IService)

public:
    virtual ~IWindowService() = default;

    virtual Vec2 get_resolution() const = 0;
    virtual uint32_t get_window_id() const = 0;
    virtual void set_title(const char *title) const = 0;

    // HACK: properly implement later
    virtual Viewport *get_viewport() const = 0;
};

} // namespace ncore
