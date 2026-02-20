#include "vxng/camera.h"
#include "webgpu/webgpu_cpp.h"

namespace vxng::camera {

Camera::Camera(glm::vec3 position, glm::mat3 rotation, float fovy_rad)
    : position(position), rotation(rotation), fovy_rad(fovy_rad) {}

Camera::Camera() : Camera(glm::vec3(0.f), glm::mat3(1.f), 0.7) {};

auto Camera::init_webgpu(wgpu::Device device) -> void {
    wgpu::Buffer buffer;
    {
        wgpu::BufferDescriptor camera_desc;
        camera_desc.label = "Camera uniform buffer";
        camera_desc.size = 144;
        camera_desc.usage =
            wgpu::BufferUsage::Uniform | wgpu::BufferUsage::CopyDst;
        buffer = device.CreateBuffer(&camera_desc);
    }

    this->wgpu.initialized = true;
    this->wgpu.device = device;
    this->wgpu.uniform_buffer = buffer;

    update_webgpu();
}

auto Camera::get_buffer() const -> wgpu::Buffer {
    return this->wgpu.uniform_buffer.Get();
}

Camera::~Camera() {
    if (!this->wgpu.initialized)
        return;

    this->wgpu.uniform_buffer.Destroy();
};

auto Camera::update_webgpu() -> void {
    if (!this->wgpu.initialized)
        return;

    // compute model matrix and inverse
    glm::mat4 inv_view = glm::mat4(this->rotation);
    inv_view[3] = glm::vec4(this->position, 1.0f);
    glm::mat4 view = glm::inverse(inv_view);

    // upload our data to GPU side
    wgpu::Queue queue = this->wgpu.device.GetQueue();
    queue.WriteBuffer(this->wgpu.uniform_buffer, 0, &view, sizeof(glm::mat4));
    queue.WriteBuffer(this->wgpu.uniform_buffer, sizeof(glm::mat4), &inv_view,
                      sizeof(glm::mat4));
    queue.WriteBuffer(this->wgpu.uniform_buffer, sizeof(glm::mat4) * 2,
                      &this->fovy_rad, sizeof(float));
}

} // namespace vxng::camera
