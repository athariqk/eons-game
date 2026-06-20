#pragma once

#include <modules/video/camera.h>
#include <ncore/kernel/structures.h>
#include <ncore/runtime/ecs/ecs_entity.h>
#include <ncore/runtime/ecs/ecs_system.h>

namespace ncore {

class Viewport;
class EcsInputSystem;

struct CameraComponent {
    float zoom = 1.0f;
    bool active = true;
};

struct CameraConfig {
    float Acceleration = 80.0f;
    float Friction = 8.0f;
    float MaxSpeed = 50.0f;
    float DragSensitivity = 25.0f;
    float ZoomSensitivity = 2.0f;
    float ZoomFriction = 10.0f;
    float MinZoom = 0.1f;
    float MaxZoom = 4.0f;
};

class EcsCameraSystem : public EcsSystem {
    NCLASS(EcsCameraSystem, EcsSystem)

public:
    EcsCameraSystem() { set_priority(-40); }

    void on_init(EcsWorld &world) override;
    void on_variable_update(EcsWorld &world, double delta) override;

private:
    void update_cam_ctrl(float delta);
    void update_cam_movement(float delta);

    EcsInputSystem *inputs = nullptr;
    Viewport *viewport = nullptr;
    EcsEntity camera_entity = 0;
    CameraConfig cfg_;
    bool is_dragging_ = false;
    Vec2 cam_input_dir_;
    Vec2 cam_velocity_;
    float cam_input_zoom_ = 0.0f;
    float cam_velocity_zoom_ = 0.0f;
};

} // namespace ncore
