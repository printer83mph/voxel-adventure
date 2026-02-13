#include "orbit-camera.h"

#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/euler_angles.hpp>

namespace vxng::scene {

OrbitCamera::OrbitCamera(glm::vec3 target, glm::vec3 angle_euler_yxz,
                         float distance, float sensitivity, float min_x,
                         float max_x)
    : Camera(), target(target), angle_euler_yxz(angle_euler_yxz),
      distance(distance) {
    settings.sensitivity = sensitivity;
    settings.min_x = min_x;
    settings.max_x = max_x;
}

OrbitCamera::OrbitCamera()
    : OrbitCamera(glm::vec3(0), glm::vec3(0.7f, 0.5f, 0.f), 8.f, 0.1f,
                  glm::pi<float>(), -glm::pi<float>()) {};

OrbitCamera::~OrbitCamera() = default;

auto OrbitCamera::on_rotate(float dx, float dy) -> void {
    this->angle_euler_yxz.y += dx * this->settings.sensitivity;
    this->angle_euler_yxz.x += dy * this->settings.sensitivity;

    this->update_camera();
    this->update_gl();
}

auto OrbitCamera::on_zoom(float delta) -> void {
    this->distance -= delta;

    this->update_camera();
    this->update_gl();
}

auto OrbitCamera::update_camera() -> void {
    this->rotation = glm::orientate3(this->angle_euler_yxz);
    this->position =
        this->target + glm::vec3(0.f, 0.f, -this->distance) * this->rotation;
}

} // namespace vxng::scene