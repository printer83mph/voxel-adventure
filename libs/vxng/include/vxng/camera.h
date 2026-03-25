#pragma once

#include "vxng/geometry.h"

#include <glm/glm.hpp>
#include <webgpu/webgpu_cpp.h>

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
    auto init_webgpu(wgpu::Device device) -> void;
    auto get_buffer() const -> wgpu::Buffer;

    /** Creates world-space ray based on an NDC screen position */
    auto screen_to_ray(glm::vec2 screen_pos) const -> geometry::Ray;

    /** Updates the aspect ratio (width / height) for screen-to-ray conversion
     */
    auto set_aspect_ratio(float aspect_ratio) -> void;

  protected:
    Camera(glm::vec3 position, glm::mat3 rotation, float fovy_rad);
    Camera();
    ~Camera();

    /** Updates buffer, should be called after any `set_` method */
    auto update_webgpu() -> void;

    glm::vec3 position;
    glm::mat3 rotation;
    float fovy_rad;
    float aspect_ratio;

  private:
    struct {
        bool initialized;
        wgpu::Device device;
        wgpu::Buffer uniform_buffer; // structure below
        /*
         * mat4 view matrix (world-to-camera)
         * mat4 inv view matrix (camera-to-world)
         * float fovy (radians)
         */
    } wgpu;
};

} // namespace vxng::camera
