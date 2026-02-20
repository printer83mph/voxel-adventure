#include "vxng/renderer.h"

#include "scene/chunk.h"
#include "wgsl/shaders.h"

#include <array>
#include <glm/glm.hpp>
#include <webgpu/webgpu_cpp.h>

#include <iostream>

namespace vxng {

Renderer::Renderer() {};
Renderer::~Renderer() {
    // WebGPU objects are automatically released when their reference counted
    // handles all go out of scope
};

typedef struct WgslGlobalsUniforms {
    float aspectRatio;
} WgslGlobalsUniforms;

typedef struct WgslCameraUniforms {
    glm::mat4 viewMat;
    glm::mat4 invViewMat;
    float fovYRad;
} WgslCameraUniforms;

auto Renderer::init_webgpu(wgpu::Device device) -> bool {

    // create uniform buffers
    wgpu::Buffer globals_buffer;
    {
        wgpu::BufferDescriptor globals_desc;
        globals_desc.label = "Scene globals uniform buffer";
        globals_desc.size = 4;
        globals_desc.usage =
            wgpu::BufferUsage::Uniform | wgpu::BufferUsage::CopyDst;
        globals_buffer = device.CreateBuffer(&globals_desc);
    }

    // create bind group layouts for shaders
    wgpu::BindGroupLayout globals_bind_group_layout = nullptr;
    wgpu::BindGroupLayout camera_bind_group_layout = nullptr;
    wgpu::BindGroupLayout chunk_bind_group_layout =
        scene::Chunk::get_bindgroup_layout(device);
    {
        // globals bind group layout (group 0)
        wgpu::BindGroupLayoutEntry globals_layout_entry;
        globals_layout_entry.binding = 0;
        globals_layout_entry.visibility = wgpu::ShaderStage::Fragment;
        globals_layout_entry.buffer.type = wgpu::BufferBindingType::Uniform;
        globals_layout_entry.buffer.minBindingSize = 4;

        wgpu::BindGroupLayoutDescriptor globals_bgl_desc;
        globals_bgl_desc.label = "Globals bind group layout";
        globals_bgl_desc.entryCount = 1;
        globals_bgl_desc.entries = &globals_layout_entry;
        globals_bind_group_layout =
            device.CreateBindGroupLayout(&globals_bgl_desc);

        // camera bind group layout (group 1)
        wgpu::BindGroupLayoutEntry camera_layout_entry;
        camera_layout_entry.binding = 0;
        camera_layout_entry.visibility = wgpu::ShaderStage::Fragment;
        camera_layout_entry.buffer.type = wgpu::BufferBindingType::Uniform;
        camera_layout_entry.buffer.minBindingSize = 144;

        wgpu::BindGroupLayoutDescriptor camera_bgl_desc;
        camera_bgl_desc.label = "Camera bind group layout";
        camera_bgl_desc.entryCount = 1;
        camera_bgl_desc.entries = &camera_layout_entry;
        camera_bind_group_layout =
            device.CreateBindGroupLayout(&camera_bgl_desc);
    }

    // create globals bind group (camera bind group created in
    // set_active_camera)
    wgpu::BindGroup globals_bind_group = nullptr;
    {
        wgpu::BindGroupEntry globals_entry;
        globals_entry.binding = 0;
        globals_entry.buffer = globals_buffer;
        globals_entry.offset = 0;
        globals_entry.size = 4;

        wgpu::BindGroupDescriptor bg_desc;
        bg_desc.layout = globals_bind_group_layout;
        bg_desc.entryCount = 1;
        bg_desc.entries = &globals_entry;
        globals_bind_group = device.CreateBindGroup(&bg_desc);
    }

    // create shader module
    wgpu::ShaderModule shader_module = nullptr;
    {
        wgpu::ShaderSourceWGSL wgsl_source;
        wgsl_source.code = vxng::shaders::FULLSCREEN_WGSL.c_str();

        wgpu::ShaderModuleDescriptor shader_desc;
        shader_desc.nextInChain = &wgsl_source;
        shader_desc.label = "Fullscreen raymarching shader";

        shader_module = device.CreateShaderModule(&shader_desc);
        if (!shader_module) {
            std::cout << "Failed to create shader module!" << std::endl;
            return false;
        }
    }

    // create pipeline layout
    wgpu::PipelineLayout pipeline_layout = nullptr;
    {
        std::array<wgpu::BindGroupLayout, 3> bind_group_layouts = {
            globals_bind_group_layout, camera_bind_group_layout,
            chunk_bind_group_layout};

        wgpu::PipelineLayoutDescriptor layout_desc;
        layout_desc.label = "Render pipeline layout";
        layout_desc.bindGroupLayoutCount = bind_group_layouts.size();
        layout_desc.bindGroupLayouts = bind_group_layouts.data();
        pipeline_layout = device.CreatePipelineLayout(&layout_desc);
    }

    // create render pipeline
    wgpu::RenderPipeline render_pipeline = nullptr;
    {
        wgpu::RenderPipelineDescriptor pipeline_desc;
        pipeline_desc.label = "Fullscreen render pipeline";
        pipeline_desc.layout = pipeline_layout;

        wgpu::VertexState vertex_state;
        vertex_state.module = shader_module;
        vertex_state.entryPoint = "vs_main";
        vertex_state.bufferCount = 0;
        vertex_state.buffers = nullptr;
        pipeline_desc.vertex = vertex_state;

        wgpu::PrimitiveState primitive_state;
        primitive_state.topology = wgpu::PrimitiveTopology::TriangleList;
        primitive_state.stripIndexFormat = wgpu::IndexFormat::Undefined;
        primitive_state.frontFace = wgpu::FrontFace::CCW;
        primitive_state.cullMode = wgpu::CullMode::None;
        pipeline_desc.primitive = primitive_state;

        wgpu::FragmentState fragment_state;
        fragment_state.module = shader_module;
        fragment_state.entryPoint = "fs_main";

        // color target
        wgpu::ColorTargetState color_target;
        color_target.format = wgpu::TextureFormat::BGRA8Unorm;
        color_target.writeMask = wgpu::ColorWriteMask::All;

        wgpu::BlendState blend_state;
        blend_state.color.srcFactor = wgpu::BlendFactor::One;
        blend_state.color.dstFactor = wgpu::BlendFactor::Zero;
        blend_state.color.operation = wgpu::BlendOperation::Add;
        blend_state.alpha.srcFactor = wgpu::BlendFactor::One;
        blend_state.alpha.dstFactor = wgpu::BlendFactor::Zero;
        blend_state.alpha.operation = wgpu::BlendOperation::Add;
        color_target.blend = &blend_state;

        fragment_state.targetCount = 1;
        fragment_state.targets = &color_target;
        pipeline_desc.fragment = &fragment_state;

        // no depth/stencil for fullscreen pass
        pipeline_desc.depthStencil = nullptr;

        // multisample state
        wgpu::MultisampleState multisample_state;
        multisample_state.count = 1;
        multisample_state.mask = ~0u;
        multisample_state.alphaToCoverageEnabled = false;
        pipeline_desc.multisample = multisample_state;

        render_pipeline = device.CreateRenderPipeline(&pipeline_desc);
        if (!render_pipeline) {
            std::cerr << "Failed to create render pipeline!" << std::endl;
            return false;
        }
    }

    // store all objects in member struct
    this->wgpu.initialized = true;
    this->wgpu.device = device;
    this->wgpu.queue = device.GetQueue();
    this->wgpu.globals_uniforms_buffer = globals_buffer;
    this->wgpu.globals_bind_group_layout = globals_bind_group_layout;
    this->wgpu.camera_bind_group_layout = camera_bind_group_layout;
    this->wgpu.globals_bind_group = globals_bind_group;
    // camera_bind_group is set in set_active_camera
    this->wgpu.shader_module = shader_module;
    this->wgpu.pipeline_layout = pipeline_layout;
    this->wgpu.render_pipeline = render_pipeline;

    return true;
}

auto Renderer::resize(int width, int height) -> void {
    float aspect = (float)width / (float)height;
    this->wgpu.queue.WriteBuffer(this->wgpu.globals_uniforms_buffer, 0, &aspect,
                                 sizeof(float));
};

auto Renderer::set_scene(const vxng::scene::Scene *scene) -> void {
    this->active_scene = scene;
};

auto Renderer::set_active_camera(const vxng::camera::Camera *camera) -> void {
    this->active_camera = camera;

    // create a new bind group for this camera's buffer
    wgpu::BindGroupEntry camera_entry;
    camera_entry.binding = 0;
    camera_entry.buffer = camera->get_buffer();
    camera_entry.offset = 0;
    camera_entry.size = 144;

    wgpu::BindGroupDescriptor bg_desc;
    bg_desc.layout = this->wgpu.camera_bind_group_layout;
    bg_desc.entryCount = 1;
    bg_desc.entries = &camera_entry;
    this->wgpu.camera_bind_group = this->wgpu.device.CreateBindGroup(&bg_desc);
}

auto Renderer::render(wgpu::RenderPassEncoder &render_pass) const -> void {
    // set the render pipeline
    render_pass.SetPipeline(this->wgpu.render_pipeline);

    // bind the uniform bind groups
    render_pass.SetBindGroup(0, this->wgpu.globals_bind_group);
    render_pass.SetBindGroup(1, this->wgpu.camera_bind_group);

    for (auto &[coord, chunk] : this->active_scene->get_chunks()) {
        render_pass.SetBindGroup(2, chunk->get_bindgroup());

        // draw fullscreen triangle (3 vertices, 1 instance)
        render_pass.Draw(3, 1, 0, 0);
    }
};

} // namespace vxng
