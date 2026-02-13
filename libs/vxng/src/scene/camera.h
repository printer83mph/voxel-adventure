#pragma once

#include <glm/glm.hpp>

namespace vxng::scene {

class Camera {
  public:
    Camera(glm::vec3 position, glm::mat3 rotation, float fovy_rad);
    Camera();
    ~Camera();

    // Update various camera parameters
    auto set_position(glm::vec3 position) -> void;
    auto set_rotation(glm::mat3 rotation) -> void;
    auto set_fovy(float fov_rad) -> void;

    /** Updates buffer, should be called after any `set_` method */
    auto update_gl() -> void;

    /** Creates buffer for use in shader programs */
    auto init_gl() -> void;

  private:
    glm::vec3 position;
    glm::mat3 rotation;
    float fovy_rad, near_plane, far_plane;

    struct {
        bool initialized;
        // GL pointers
    } gl;
};

} // namespace vxng::scene