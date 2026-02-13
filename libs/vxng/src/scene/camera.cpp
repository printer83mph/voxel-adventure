#include "camera.h"

#include "glm/fwd.hpp"
#include "util.h"

namespace vxng::scene {

Camera::Camera(glm::vec3 position, glm::mat3 rotation, float fovy_rad)
    : position(position), rotation(rotation), fovy_rad(fovy_rad) {}

Camera::Camera() : Camera(glm::vec3(0.f), glm::mat3(1.f), 0.7) {};

Camera::~Camera() {
    if (!this->gl.initialized)
        return;

    // TODO: cleanup GL things
};

auto Camera::set_rotation(glm::mat3 rotation) -> void {
    // TODO: Implement rotation logic
    throw not_implemented_error();
}

auto Camera::set_position(glm::vec3 position) -> void {
    // TODO: Implement position logic
    throw not_implemented_error();
}

auto Camera::set_fovy(float fov_rad) -> void {
    // TODO: Implement field of view logic
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