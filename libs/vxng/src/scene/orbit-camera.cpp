#include "orbit-camera.h"

namespace vxng::scene {

OrbitCamera::OrbitCamera(float distance, glm::vec3 angle_euler_xyz,
                         float sensitivity, float min_x, float max_x)
    : Camera(), distance(distance), angle_euler_xyz(angle_euler_xyz) {
    settings.sensitivity = sensitivity;
    settings.min_x = min_x;
    settings.max_x = max_x;
}

OrbitCamera::OrbitCamera()
    : OrbitCamera(8.f, glm::vec3(0.7f, 0.5f, 0.f), 0.1f, glm::pi<float>(),
                  -glm::pi<float>()) {};

OrbitCamera::~OrbitCamera() = default;

auto OrbitCamera::on_rotate(float dx, float dy) -> void {
    this->angle_euler_xyz.y += dx * this->settings.sensitivity;
    this->angle_euler_xyz.x += dy * this->settings.sensitivity;

    this->update_camera();
    this->update_gl();
}

auto OrbitCamera::on_zoom(float delta) -> void {
    this->distance -= delta;

    this->update_camera();
    this->update_gl();
}

auto OrbitCamera::update_camera() -> void {
    // TODO: implement
}

} // namespace vxng::scene