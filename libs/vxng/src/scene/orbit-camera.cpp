#include "orbit-camera.h"

#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/euler_angles.hpp>

namespace vxng::scene {

OrbitCamera::OrbitCamera(glm::vec3 target, glm::vec3 angle_euler_yxz,
                         float distance, float fovy_rad)
    : Camera(glm::vec3(0.f), glm::mat3(1.f), fovy_rad), target(target),
      angle_euler_yxz(angle_euler_yxz), distance(distance) {
    this->update_camera();
}

OrbitCamera::OrbitCamera()
    : OrbitCamera(glm::vec3(0), glm::vec3(0.7f, 0.5f, 0.f), 8.f, 0.7f) {};

OrbitCamera::~OrbitCamera() = default;

auto OrbitCamera::on_rotate(float dx, float dy) -> void {
    // clamp x rotation to not go upside down
    this->angle_euler_yxz.x = glm::clamp(
        this->angle_euler_yxz.y + dy * this->settings.rotate_sensitivity,
        this->settings.min_x, this->settings.max_x);

    this->angle_euler_yxz.y += dx * this->settings.rotate_sensitivity;

    this->update_camera();
    this->update_gl();
}

auto OrbitCamera::on_zoom(float delta) -> void {
    this->distance -= delta;

    this->update_camera();
    this->update_gl();
}

auto OrbitCamera::on_pan(float dx, float dy) -> void {
    glm::vec3 right = this->rotation[0];
    glm::vec3 up = this->rotation[1];

    this->target += (right * dx + up * dy) * this->settings.pan_sensitivity;

    this->update_camera();
    this->update_gl();
}

auto OrbitCamera::update_camera() -> void {
    this->rotation = glm::orientate3(this->angle_euler_yxz);
    this->position = this->target - this->rotation[2] * this->distance;
}

} // namespace vxng::scene