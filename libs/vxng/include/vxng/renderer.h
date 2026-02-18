#pragma once

// #include "vxng/camera.h"
#include "vxng/scene.h"

#include <webgpu/webgpu_cpp.h>

namespace vxng {

/**
 * Abstracted object for
 */
class Renderer {
  public:
    Renderer();
    ~Renderer();

    /** Sets up program + shader bindings */
    auto init_webgpu(wgpu::Device *device) -> bool;
    auto resize(int width, int height) -> void;
    auto set_scene(vxng::scene::Scene const *scene) -> void;
    // auto set_active_camera(const vxng::camera::Camera *camera) -> void;
    auto render() const -> void;

  private:
    struct {
        bool initialized;
        wgpu::Device *device;
        wgpu::Buffer globals_uniforms_buffer, camera_uniforms_buffer;
    } wgpu;

    // const vxng::camera::Camera *active_camera;
};

} // namespace vxng
