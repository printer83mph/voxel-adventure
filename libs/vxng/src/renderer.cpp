#include "vxng/renderer.h"

#include "gl-util.h"
#include "glsl/shaders.h"
#include "util.h"

#include <GL/glew.h>
#include <iostream>

namespace vxng {

Renderer::Renderer() {};
Renderer::~Renderer() {};

auto Renderer::init_gl() -> bool {
    // create opengl program
    GLuint gl_program = glCreateProgram();

    // create vert shader
    GLuint vert_shader = glCreateShader(GL_VERTEX_SHADER);
    {
        const GLchar *vert_src[] = {vxng::shaders::FULLSCREEN_VERT.c_str()};
        glShaderSource(vert_shader, 1, vert_src, NULL);
        glCompileShader(vert_shader);

        // check for errors
        GLint vert_compiled = GL_FALSE;
        glGetShaderiv(vert_shader, GL_COMPILE_STATUS, &vert_compiled);
        if (vert_compiled != GL_TRUE) {
            std::cout << "Unable to compile vertex shader!" << std::endl;
            printShaderLog(vert_shader);
            return false;
        }

        glAttachShader(gl_program, vert_shader);
    }

    // create frag shader
    GLuint frag_shader = glCreateShader(GL_FRAGMENT_SHADER);
    {
        const GLchar *frag_src[] = {vxng::shaders::FULLSCREEN_FRAG.c_str()};
        glShaderSource(frag_shader, 1, frag_src, NULL);
        glCompileShader(frag_shader);

        // check for errors
        GLint vert_compiled = GL_FALSE;
        glGetShaderiv(frag_shader, GL_COMPILE_STATUS, &vert_compiled);
        if (vert_compiled != GL_TRUE) {
            std::cout << "Unable to compile fragment shader!" << std::endl;
            printShaderLog(frag_shader);
            return false;
        }

        glAttachShader(gl_program, frag_shader);
    }

    glLinkProgram(gl_program);

    // check for errors
    GLint program_success = GL_TRUE;
    glGetProgramiv(gl_program, GL_LINK_STATUS, &program_success);
    if (program_success != GL_TRUE) {
        std::cout << "Error linking program!" << std::endl;
        printProgramLog(gl_program);
        return false;
    }

    this->gl.program = gl_program;
    this->gl.vert_shader = vert_shader;
    this->gl.frag_shader = frag_shader;

    return true;
}

auto Renderer::set_scene(const vxng::Scene *scene) -> void {
    throw not_implemented_error();
};

auto Renderer::render() const -> void {
    glUseProgram(this->gl.program);
    // don't use any buffers, we just let our vert shader set gl_Position
    glDrawArrays(GL_TRIANGLES, 0, 3);
    glUseProgram(NULL);
};

} // namespace vxng
