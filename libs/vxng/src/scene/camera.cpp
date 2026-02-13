#include "camera.h"

#include "util.h"

namespace vxng::scene {

Camera::Camera(glm::vec3 position, glm::mat3 rotation, float fovy_rad)
    : position(position), rotation(rotation), fovy_rad(fovy_rad) {}

Camera::Camera() : Camera(glm::vec3(0.f), glm::mat3(1.f), 0.7) {};

auto Camera::init_gl() -> void {
    glGenBuffers(1, &this->gl.ubo);
    glBindBuffer(GL_UNIFORM_BUFFER, this->gl.ubo);
    glBufferData(GL_UNIFORM_BUFFER, sizeof(glm::mat4) * 2 + sizeof(float),
                 nullptr, GL_DYNAMIC_DRAW);
    glBindBuffer(GL_UNIFORM_BUFFER, 0);

    this->gl.initialized = true;
    // update buffer to match current CPU values
    this->update_gl();
}

Camera::~Camera() {
    if (!this->gl.initialized)
        return;

    // cleanup openGL stuffs
    glDeleteBuffers(1, &this->gl.ubo);
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
    if (!this->gl.initialized)
        return;

    // compute model matrix and inverse
    glm::mat4 model = glm::mat4(this->rotation);
    model[3] = glm::vec4(this->position, 1.0f);
    glm::mat4 inv_model = glm::inverse(model);

    // upload our data to GPU side
    glBindBuffer(GL_UNIFORM_BUFFER, this->gl.ubo);
    glBufferSubData(GL_UNIFORM_BUFFER, 0, sizeof(glm::mat4), &model);
    glBufferSubData(GL_UNIFORM_BUFFER, sizeof(glm::mat4), sizeof(glm::mat4),
                    &inv_model);
    glBufferSubData(GL_UNIFORM_BUFFER, sizeof(glm::mat4) * 2, sizeof(float),
                    &this->fovy_rad);
    glBindBuffer(GL_UNIFORM_BUFFER, 0);
}

} // namespace vxng::scene