#pragma once

#include <ncore/core/vector.h>

namespace nc {

struct PhysicsDebugDraw {
    void ( *draw_polygon )( const Vec2f* vertices, int vertexCount, uint32_t color, void* context ) = nullptr;
    void ( *draw_solid_polygon )( const Vec2f* vertices, int vertexCount, float radius, uint32_t color, void* context ) =
        nullptr;
    void ( *draw_circle )( Vec2f center, float radius, uint32_t color, void* context )             = nullptr;
    void ( *draw_solid_circle )( Vec2f center, float radius, uint32_t color, void* context )       = nullptr;
    void ( *draw_solid_capsule )( Vec2f p1, Vec2f p2, float radius, uint32_t color, void* context ) = nullptr;
    void ( *draw_segment )( Vec2f p1, Vec2f p2, uint32_t color, void* context )                     = nullptr;
    void ( *draw_transform )( Vec2f position, float angle, void* context )                         = nullptr;
    void ( *draw_point )( Vec2f p, float size, uint32_t color, void* context )                     = nullptr;
    void* context                                                                                 = nullptr;
    bool draw_shapes                                                                              = true;
    bool draw_joints                                                                              = true;
    bool draw_aabbs                                                                               = false;
    bool draw_mass                                                                                = false;
    bool draw_contacts                                                                            = false;
};

} // namespace nc
