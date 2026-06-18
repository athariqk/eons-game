#include <runtime/ecs/ecs_camera_system.h>

#include <modules/ecs/ecs_world.h>
#include <modules/video/viewport.h>
#include <runtime/ecs/ecs_input_system.h>
#include <runtime/ecs/ecs_transform.h>

namespace ncore {

void EcsCameraSystem::on_init(EcsWorld &world) {
    inputs = world.get_system<EcsInputSystem>();
    viewport = world.get_viewport();

    // Create camera entity in the world
    camera_entity = world.create_entity();
    world.set_component(camera_entity, EcsTransform{Vec2{0, 0}});
    world.set_component(camera_entity, CameraComponent{1.0f, true});
}

void EcsCameraSystem::on_variable_update(EcsWorld &world, double delta) {
    if (!inputs || !viewport)
        return;

    float dt = static_cast<float>(delta);
    update_cam_ctrl(dt);
    update_cam_movement(dt);
}

void EcsCameraSystem::update_cam_ctrl(float delta) {
    cam_input_dir_.zero();

    if (inputs->get_is_key_pressed(KeyboardEvent::Key::W))
        cam_input_dir_.y -= 1.0f;
    if (inputs->get_is_key_pressed(KeyboardEvent::Key::S))
        cam_input_dir_.y += 1.0f;
    if (inputs->get_is_key_pressed(KeyboardEvent::Key::A))
        cam_input_dir_.x -= 1.0f;
    if (inputs->get_is_key_pressed(KeyboardEvent::Key::D))
        cam_input_dir_.x += 1.0f;

    float len_sq = cam_input_dir_.length_sqr();
    if (len_sq > 0.001f)
        cam_input_dir_ *= 1.0f / std::sqrt(len_sq);

    is_dragging_ = inputs->get_is_mouse_button_pressed(ButtonIndex::MIDDLE);

    if (is_dragging_) {
        float zoom = viewport->get_camera().get_zoom();
        auto md = inputs->get_last_mouse_delta();

        auto target_vel = (-md / (1.0f * zoom)) / delta;
        float t = 1.0f - std::exp(-cfg_.DragSensitivity * delta);
        cam_velocity_ += (target_vel - cam_velocity_) * t;

        cam_input_dir_.zero();
    }

    cam_input_zoom_ = inputs->get_last_mouse_wheel_delta().y;
}

void EcsCameraSystem::update_cam_movement(float delta) {
    if (!is_dragging_) {
        cam_velocity_ += cam_input_dir_ * cfg_.Acceleration * delta;

        float friction = 1.0f - std::exp(-cfg_.Friction * delta);
        cam_velocity_ -= cam_velocity_ * friction;

        float speed_sq = cam_velocity_.length_sqr();
        if (speed_sq > cfg_.MaxSpeed * cfg_.MaxSpeed)
            cam_velocity_ *= cfg_.MaxSpeed / std::sqrt(speed_sq);
    }

    auto pos = viewport->get_camera().get_position();
    pos += cam_velocity_ * delta;
    viewport->set_camera_position(pos);

    cam_velocity_zoom_ += cam_input_zoom_ * cfg_.ZoomSensitivity;
    float zoom_friction = 1.0f - std::exp(-cfg_.ZoomFriction * delta);
    cam_velocity_zoom_ -= cam_velocity_zoom_ * zoom_friction;

    float new_zoom =
        std::clamp(viewport->get_camera().get_zoom() + cam_velocity_zoom_ * delta, cfg_.MinZoom, cfg_.MaxZoom);
    viewport->set_camera_zoom(new_zoom);
}

} // namespace ncore
