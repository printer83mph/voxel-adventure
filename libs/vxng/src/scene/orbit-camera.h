#pragma once

#include "camera.h"

#include <glm/ext/scalar_constants.hpp>

namespace vxng::scene {

class OrbitCamera : vxng::scene::Camera {
  public:
    OrbitCamera(float distance, glm::vec3 angle_euler_xyz, float sensitivity,
                float min_x, float max_x);
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

    glm::vec3 angle_euler_xyz;
    float distance;
};

} // namespace vxng::scene