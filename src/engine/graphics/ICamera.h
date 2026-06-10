#pragma once

#include "Vec2D.h"

namespace ncore {

//! \brief Camera interface for 2D rendering
//! Defines the view transformation (what to see in the world)
class ICamera {
public:
    virtual ~ICamera() = default;

    //! \brief Get camera position in world coordinates
    virtual Vec2D get_position() const = 0;

    //! \brief Get camera zoom level (1.0 = normal, >1.0 = zoomed in, <1.0 = zoomed out)
    virtual float get_zoom() const = 0;

    //! \brief Set camera position
    virtual void set_position(const Vec2D &position) = 0;

    //! \brief Set camera zoom
    virtual void set_zoom(float zoom) = 0;
};

} // namespace ncore
