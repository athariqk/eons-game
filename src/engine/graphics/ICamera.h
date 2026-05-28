#pragma once

#include "Vector2D.h"

//! \brief Camera interface for 2D rendering
//! Defines the view transformation (what to see in the world)
class ICamera {
public:
    virtual ~ICamera() = default;

    //! \brief Get camera position in world coordinates
    virtual Vector2D GetPosition() const = 0;

    //! \brief Get camera zoom level (1.0 = normal, >1.0 = zoomed in, <1.0 = zoomed out)
    virtual float GetZoom() const = 0;

    //! \brief Set camera position
    virtual void SetPosition(const Vector2D &position) = 0;

    //! \brief Set camera zoom
    virtual void SetZoom(float zoom) = 0;
};
