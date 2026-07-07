#pragma once

#include <ncore/kernel/structures.h>

namespace nc {

struct PhysicsDebugDraw {
    void ( *draw_polygon )( const Vec2* vertices, int vertexCount, uint32_t color, void* context ) = nullptr;
    void ( *draw_solid_polygon )( const Vec2* vertices, int vertexCount, float radius, uint32_t color, void* context ) =
        nullptr;
    void ( *draw_circle )( Vec2 center, float radius, uint32_t color, void* context )             = nullptr;
    void ( *draw_solid_circle )( Vec2 center, float radius, uint32_t color, void* context )       = nullptr;
    void ( *draw_solid_capsule )( Vec2 p1, Vec2 p2, float radius, uint32_t color, void* context ) = nullptr;
    void ( *draw_segment )( Vec2 p1, Vec2 p2, uint32_t color, void* context )                     = nullptr;
    void ( *draw_transform )( Vec2 position, float angle, void* context )                         = nullptr;
    void ( *draw_point )( Vec2 p, float size, uint32_t color, void* context )                     = nullptr;
    void* context                                                                                 = nullptr;
    bool draw_shapes                                                                              = true;
    bool draw_joints                                                                              = true;
    bool draw_aabbs                                                                               = false;
    bool draw_mass                                                                                = false;
    bool draw_contacts                                                                            = false;
};

} // namespace nc
