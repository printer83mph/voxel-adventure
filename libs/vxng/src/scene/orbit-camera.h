#pragma once

#include "camera.h"

#include <glm/ext/scalar_constants.hpp>

namespace vxng::scene {

class OrbitCamera : vxng::scene::Camera {
  public:
    auto on_mouse_rotate(float dx, float dy);

    /** Tweakable settings */
    struct {
        float sensitivity = 0.1f;
        float min_x = glm::pi<float>();
        float max_x = -glm::pi<float>();
    } settings;
};

} // namespace vxng::scene