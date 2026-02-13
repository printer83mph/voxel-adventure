#pragma once

#include "camera.h"

#include <glm/ext/scalar_constants.hpp>

namespace vxng::scene {

class OrbitCamera : vxng::scene::Camera {
  public:
    OrbitCamera(glm::vec3 target, glm::vec3 angle_euler_yxz, float distance,
                float sensitivity, float min_x, float max_x);
    OrbitCamera();
    ~OrbitCamera();

    auto on_rotate(float dx, float dy) -> void;
    auto on_zoom(float delta) -> void;

    /** Tweakable settings */
    struct {
        float sensitivity;
        float min_x, max_x;
    } settings;

  private:
    auto update_camera() -> void;

    glm::vec3 target;
    glm::vec3 angle_euler_yxz;
    float distance;
};

} // namespace vxng::scene