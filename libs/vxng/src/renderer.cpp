#include "vxng/renderer.h"

#include "util.h"
#include "wgsl/shaders.h"

#include <array>
#include <glm/glm.hpp>
#include <webgpu/webgpu_cpp.h>

#include <iostream>

#define UNIF_BIND_GLOBALS 0
#define UNIF_BIND_CAMERA 1

namespace vxng {

Renderer::Renderer() {};
Renderer::~Renderer() {
    // WebGPU objects are automatically released when their handles go out of
    // scope
};

typedef struct WgslGlobalsUniforms {
    float aspectRatio;
} WgslGlobalsUniforms;

typedef struct WgslCameraUniforms {
    glm::mat4 viewMat;
    glm::mat4 invViewMat;
    float fovYRad;
} WgslCameraUniforms;

auto Renderer::init_webgpu(wgpu::Device *device) -> bool {

    // create uniform buffers
    wgpu::Buffer globals_buffer, camera_buffer;
    {
        wgpu::BufferDescriptor globals_desc;
        globals_desc.label = "Scene globals uniform buffer";
        globals_desc.size = 4;
        globals_desc.usage =
            wgpu::BufferUsage::Uniform | wgpu::BufferUsage::CopyDst;
        globals_buffer = device->CreateBuffer(&globals_desc);

        wgpu::BufferDescriptor camera_desc;
        camera_desc.label = "Camera uniform buffer";
        camera_desc.size = 144;
        camera_desc.usage =
            wgpu::BufferUsage::Uniform | wgpu::BufferUsage::CopyDst;
        camera_buffer = device->CreateBuffer(&camera_desc);
    }

    // create bind group layout for shaders
    wgpu::BindGroupLayout bind_group_layout = nullptr;
    {
        std::array<wgpu::BindGroupLayoutEntry, 2> bgl_entries;

        wgpu::BindGroupLayoutEntry &globals_layout = bgl_entries[0];
        globals_layout.binding = 0;
        globals_layout.visibility = wgpu::ShaderStage::Fragment;
        globals_layout.buffer.type = wgpu::BufferBindingType::Uniform;
        globals_layout.buffer.minBindingSize = 4;

        wgpu::BindGroupLayoutEntry &camera_layout = bgl_entries[1];
        camera_layout.binding = 1;
        camera_layout.visibility = wgpu::ShaderStage::Fragment;
        camera_layout.buffer.type = wgpu::BufferBindingType::Uniform;
        camera_layout.buffer.minBindingSize = 144;

        wgpu::BindGroupLayoutDescriptor bgl_desc;
        bgl_desc.label = "Render uniforms binding group";
        bgl_desc.entryCount = 2;
        bgl_desc.entries = &bgl_entries[0];
        bind_group_layout = device->CreateBindGroupLayout(&bgl_desc);
    }

    // create bind groups for shaders
    wgpu::BindGroup bind_group = nullptr;
    {
        std::array<wgpu::BindGroupEntry, 2> bg_entries;

        wgpu::BindGroupEntry &globals_entry = bg_entries[0];
        globals_entry.binding = 0;
        globals_entry.buffer = globals_buffer;
        globals_entry.offset = 0;
        globals_entry.size = 4;

        wgpu::BindGroupEntry &camera_entry = bg_entries[1];
        camera_entry.binding = 1;
        camera_entry.buffer = camera_buffer;
        camera_entry.offset = 0;
        camera_entry.size = 144;

        wgpu::BindGroupDescriptor bg_desc;
        bg_desc.layout = bind_group_layout;
        bg_desc.entryCount = 2;
        bg_desc.entries = &bg_entries[0];
        bind_group = device->CreateBindGroup(&bg_desc);
    }

    // create shader module
    wgpu::ShaderModule shader_module = nullptr;
    {
        wgpu::ShaderSourceWGSL wgsl_source;
        wgsl_source.code = vxng::shaders::FULLSCREEN_WGSL.c_str();

        wgpu::ShaderModuleDescriptor shader_desc;
        shader_desc.nextInChain = &wgsl_source;
        shader_desc.label = "Fullscreen raymarching shader";

        shader_module = device->CreateShaderModule(&shader_desc);
        if (!shader_module) {
            std::cout << "Failed to create shader module!" << std::endl;
            return false;
        }
    }

    // create pipeline layout
    wgpu::PipelineLayout pipeline_layout = nullptr;
    {
        wgpu::PipelineLayoutDescriptor layout_desc;
        layout_desc.label = "Render pipeline layout";
        layout_desc.bindGroupLayoutCount = 1;
        layout_desc.bindGroupLayouts = &bind_group_layout;
        pipeline_layout = device->CreatePipelineLayout(&layout_desc);
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

        render_pipeline = device->CreateRenderPipeline(&pipeline_desc);
        if (!render_pipeline) {
            std::cerr << "Failed to create render pipeline!" << std::endl;
            return false;
        }
    }

    // store all objects in member struct
    this->wgpu.initialized = true;
    this->wgpu.device = device;
    this->wgpu.globals_uniforms_buffer = globals_buffer;
    this->wgpu.camera_uniforms_buffer = camera_buffer;
    this->wgpu.bind_group_layout = bind_group_layout;
    this->wgpu.bind_group = bind_group;
    this->wgpu.shader_module = shader_module;
    this->wgpu.pipeline_layout = pipeline_layout;
    this->wgpu.render_pipeline = render_pipeline;

    return true;
}

auto Renderer::resize(int width, int height) -> void {
    float aspect = (float)width / (float)height;
    this->wgpu.device->GetQueue().WriteBuffer(
        this->wgpu.globals_uniforms_buffer, 0, &aspect, sizeof(float));
};

auto Renderer::set_scene(const vxng::scene::Scene *scene) -> void {
    throw not_implemented_error();
};

/*
auto Renderer::set_active_camera(const vxng::camera::Camera *camera) -> void {
    this->active_camera = camera;
    // bind this camera's ubo to our bind point
    glBindBufferBase(GL_UNIFORM_BUFFER, UNIF_BIND_CAMERA, camera->get_ubo());
}
*/

auto Renderer::render(wgpu::RenderPassEncoder &renderPass) const -> void {
    // set the render pipeline
    renderPass.SetPipeline(this->wgpu.render_pipeline);

    // bind the uniform bind group
    renderPass.SetBindGroup(0, this->wgpu.bind_group);

    // draw fullscreen triangle (3 vertices, 1 instance)
    renderPass.Draw(3, 1, 0, 0);
};

} // namespace vxng
