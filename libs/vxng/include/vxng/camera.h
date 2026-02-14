#pragma once

#include <GL/glew.h>
#include <glm/glm.hpp>

namespace vxng::camera {

/**
 * Base class representing a camera, for use in rendering with a `Scene` object.
 * Handles OpenGL buffer allocation, updates, and deallocation.
 *
 * Should be subclassed for various camera types (i.e. walk, orbit, etc).
 */
class Camera {
  public:
    /** Creates buffer for use in shader programs */
    auto init_gl() -> void;
    auto get_ubo() const -> GLuint;

  protected:
    Camera(glm::vec3 position, glm::mat3 rotation, float fovy_rad);
    Camera();
    ~Camera();

    /** Updates buffer, should be called after any `set_` method */
    auto update_gl() -> void;

    glm::vec3 position;
    glm::mat3 rotation;
    float fovy_rad;

  private:
    struct {
        bool initialized;
        GLuint ubo; // uniform buffer object, structure below
        /*
         * mat4 model matrix
         * mat4 inv model matrix
         * float fovy (radians)
         */
    } gl;
};

} // namespace vxng::camera