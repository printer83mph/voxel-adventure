#pragma once

#include "vxng/camera.h"
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
    auto init_webgpu(wgpu::Device device) -> bool;
    auto get_depth_texture_view() const -> wgpu::TextureView;
    auto resize(int width, int height) -> void;
    auto set_scene(vxng::scene::Scene const *scene) -> void;
    auto set_active_camera(const vxng::camera::Camera *camera) -> void;
    auto render(wgpu::RenderPassEncoder &render_pass) const -> void;

  private:
    auto create_depth_texture(int width, int height) -> void;

    struct {
        bool initialized;
        wgpu::Device device;
        wgpu::Queue queue;
        wgpu::Buffer globals_uniforms_buffer;
        wgpu::BindGroupLayout globals_bind_group_layout;
        wgpu::BindGroupLayout camera_bind_group_layout;
        wgpu::BindGroup globals_bind_group;
        wgpu::BindGroup camera_bind_group;
        wgpu::ShaderModule shader_module;
        wgpu::PipelineLayout pipeline_layout;
        wgpu::RenderPipeline render_pipeline;
        wgpu::Texture depth_texture;
        wgpu::TextureView depth_texture_view;
    } wgpu;

    const vxng::camera::Camera *active_camera;
    const vxng::scene::Scene *active_scene;
};

} // namespace vxng
