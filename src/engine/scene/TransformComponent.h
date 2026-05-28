#pragma once

#include "EntitySystem.h"
#include "Vector2D.h"

struct TransformComponent : Component {
    Vector2D position;

    float height = 32;
    float width = 32;
    float scale = 1;

    TransformComponent() { position.Zero(); }

    //! \brief Construct with only scale parameter
    TransformComponent(float sc) {
        position.Zero();
        scale = sc;
    }

    //! \brief  Construct with x, y position paremeter
    TransformComponent(float x, float y) {
        position.x = x;
        position.y = y;
    }

    //! \brief Construct with x, y, and scale parameter
    TransformComponent(float x, float y, float sc) {
        position.x = x;
        position.y = y;
        scale = sc;
    }

    //! \brief Construct with x, y and w, h size parameter
    TransformComponent(float x, float y, float w, float h) {
        position.x = x;
        position.y = y;
        width = w;
        height = h;
    }

    //! \brief  Construct with all transform elements parameter
    TransformComponent(float x, float y, float w, float h, float sc) {
        position.x = x;
        position.y = y;
        width = w;
        height = h;
        scale = sc;
    }
};
