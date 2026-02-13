#include "camera.h"

#include "util.h"

namespace vxng::scene {

Camera::Camera() = default;
Camera::~Camera() = default;

auto Camera::set_rotation_euler(glm::vec3 rotation) -> void {
    // TODO: Implement rotation logic
    throw not_implemented_error();
}

auto Camera::set_position(float distance) -> void {
    // TODO: Implement position logic
    throw not_implemented_error();
}

auto Camera::set_fovy(float fov_rad) -> void {
    // TODO: Implement field of view logic
    throw not_implemented_error();
}

auto Camera::set_near_far_plane(float near, float far) -> void {
    // TODO: Implement near/far plane logic
    throw not_implemented_error();
}

auto Camera::update_gl() -> void {
    // TODO: Implement GL buffer update
    throw not_implemented_error();
}

auto Camera::init_gl() -> void {
    // TODO: Implement GL buffer initialization
    throw not_implemented_error();
}

} // namespace vxng::scene