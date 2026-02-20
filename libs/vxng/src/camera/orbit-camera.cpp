#include "vxng/orbit-camera.h"

#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/euler_angles.hpp>

namespace vxng::camera {

OrbitCamera::OrbitCamera(glm::vec3 target, glm::vec3 angle_euler_yxz,
                         float distance, float fovy_rad)
    : Camera(glm::vec3(0.f), glm::mat3(1.f), fovy_rad), target(target),
      angle_euler(angle_euler_yxz), distance(distance) {
    this->update_camera();
}

OrbitCamera::OrbitCamera()
    : OrbitCamera(glm::vec3(0), glm::vec3(-0.5f, 0.5f, 0.f), 8.f, 0.7f) {};

OrbitCamera::~OrbitCamera() = default;

auto OrbitCamera::handle_rotation(float dx, float dy) -> void {
    // clamp x rotation to not go upside down
    this->angle_euler.x += -dy * this->settings.rotate_sensitivity;
    this->angle_euler.x = glm::clamp(this->angle_euler.x, this->settings.min_x,
                                     this->settings.max_x);

    // wrap y rotation to prevent accumulation issues
    this->angle_euler.y =
        glm::mod(this->angle_euler.y + -dx * this->settings.rotate_sensitivity,
                 glm::two_pi<float>());

    this->update_camera();
    this->update_webgpu();
}

auto OrbitCamera::handle_zoom(float delta) -> void {
    this->distance *= glm::exp(-delta * this->settings.zoom_sensitivity);

    this->update_camera();
    this->update_webgpu();
}

auto OrbitCamera::handle_pan(float dx, float dy) -> void {
    glm::vec3 right = this->rotation[0];
    glm::vec3 up = this->rotation[1];

    this->target += (right * -dx + up * dy) * this->settings.pan_sensitivity;

    this->update_camera();
    this->update_webgpu();
}

auto OrbitCamera::update_camera() -> void {
    this->rotation = glm::mat3(glm::eulerAngleYXZ(
        this->angle_euler.y, this->angle_euler.x, this->angle_euler.z));

    auto forward = -this->rotation[2];
    this->position = this->target - forward * this->distance;
}

} // namespace vxng::camera
