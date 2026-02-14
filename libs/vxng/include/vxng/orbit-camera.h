#pragma once

#include "camera.h"

#include <glm/ext/scalar_constants.hpp>

namespace vxng::camera {

class OrbitCamera : public vxng::camera::Camera {
  public:
    OrbitCamera(glm::vec3 target, glm::vec3 angle_euler_yxz, float distance,
                float fovy_rad);
    OrbitCamera();
    ~OrbitCamera();

    auto handle_rotation(float dx, float dy) -> void;
    auto handle_zoom(float delta) -> void;
    auto handle_pan(float dx, float dy) -> void;

    /** Tweakable settings for consumers */
    struct {
        float rotate_sensitivity = 0.05f;
        float min_x = glm::pi<float>(), max_x = -glm::pi<float>();
        float pan_sensitivity = 0.05f;
    } settings;

  private:
    /** Update base `Camera` class protected parameters */
    auto update_camera() -> void;

    glm::vec3 target;
    glm::vec3 angle_euler_yxz;
    float distance;
};

} // namespace vxng::camera