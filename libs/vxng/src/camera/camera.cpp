#include "vxng/camera.h"
#include "webgpu/webgpu_cpp.h"

namespace vxng::camera {

Camera::Camera(glm::vec3 position, glm::mat3 rotation, float fovy_rad)
    : position(position), rotation(rotation), fovy_rad(fovy_rad),
      aspect_ratio(1.0f) {}

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

auto Camera::screen_to_ray(glm::vec2 screen_pos) const -> geometry::Ray {
    // screen_pos is assumed to be in NDC coordinates [-1, 1]
    // create ray dir in camera space based on fov
    float tan_half_fovy = glm::tan(this->fovy_rad * 0.5f);
    glm::vec3 camera_dir = glm::normalize(glm::vec3(
        // ray points down negative z (into the screen)
        screen_pos.x * tan_half_fovy * this->aspect_ratio,
        screen_pos.y * tan_half_fovy, -1.0f));

    // convert to world space
    glm::vec3 world_dir = glm::normalize(this->rotation * camera_dir);

    geometry::Ray ray;
    ray.origin = this->position;
    ray.direction = world_dir;

    return ray;
}

auto Camera::get_forward() const -> glm::vec3 { return -this->rotation[2]; }

auto Camera::set_aspect_ratio(float aspect_ratio) -> void {
    this->aspect_ratio = aspect_ratio;
}

auto Camera::get_aspect_ratio() const -> float { return this->aspect_ratio; }

} // namespace vxng::camera
