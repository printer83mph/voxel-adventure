#pragma once

#include <glm/glm.hpp>

namespace vxng::scene {

class Camera {
  public:
    Camera();
    ~Camera();

    // Update various camera parameters
    auto set_rotation_euler(glm::vec3 rotation) -> void;
    auto set_position(float distance) -> void;
    auto set_fovy(float fov_rad) -> void;
    auto set_near_far_plane(float near, float far) -> void;

    /** Updates buffer, should be called after any `set_` method */
    auto update_gl() -> void;

    /** Creates buffer for use in shader programs */
    auto init_gl() -> void;

  private:
    glm::vec3 position;
    glm::mat3 rotation;
    float fovy, near_plane, far_plane;
};

} // namespace vxng::scene