#pragma once

#include "vxng/scene.h"

#include <GL/glew.h>

namespace vxng {

/**
 * Abstracted object for
 */
class Renderer {
  public:
    Renderer();
    ~Renderer();

    /** Sets up program + shader bindings */
    auto init_gl() -> bool;
    auto set_scene(vxng::Scene const *scene) -> void;
    auto render() const -> void;

  private:
    struct {
        GLuint program;
        GLuint frag_shader;
        GLuint vert_shader;
    } gl;
};

} // namespace vxng
