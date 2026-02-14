#include "vxng/renderer.h"

#include "gl-util.h"
#include "glsl/shaders.h"
#include "util.h"

#include <GL/glew.h>

#include <iostream>

#define UNIF_BIND_GLOBALS 0
#define UNIF_BIND_CAMERA 1

namespace vxng {

Renderer::Renderer() {};
Renderer::~Renderer() {
    if (!this->gl.initialized)
        return;
    glDeleteProgram(this->gl.program);
    glDeleteShader(this->gl.frag_shader);
    glDeleteShader(this->gl.vert_shader);
    glDeleteVertexArrays(1, &this->gl.vao);
};

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

    // use uniform blocks
    {
        GLuint globals_block = glGetUniformBlockIndex(gl_program, "Globals");
        GLuint camera_block = glGetUniformBlockIndex(gl_program, "Camera");
        glUniformBlockBinding(gl_program, globals_block, UNIF_BIND_GLOBALS);
        glUniformBlockBinding(gl_program, camera_block, UNIF_BIND_CAMERA);
    }

    // check for errors
    GLint program_success = GL_TRUE;
    glGetProgramiv(gl_program, GL_LINK_STATUS, &program_success);
    if (program_success != GL_TRUE) {
        std::cout << "Error linking program!" << std::endl;
        printProgramLog(gl_program);
        return false;
    }

    // create vao
    GLuint vao;
    glGenVertexArrays(1, &vao);
    glBindVertexArray(vao);

    // setup global rendering params ubo
    GLuint globals_ubo;
    {
        glGenBuffers(1, &globals_ubo);
        glBindBuffer(GL_UNIFORM_BUFFER, globals_ubo);
        glBufferData(GL_UNIFORM_BUFFER, sizeof(float), nullptr,
                     GL_DYNAMIC_DRAW);

        glBindBufferBase(GL_UNIFORM_BUFFER, UNIF_BIND_GLOBALS, globals_ubo);
    }

    this->gl.initialized = true;
    this->gl.program = gl_program;
    this->gl.vert_shader = vert_shader;
    this->gl.frag_shader = frag_shader;
    this->gl.vao = vao;
    this->gl.globals_ubo = globals_ubo;

    return true;
}

auto Renderer::resize(int width, int height) -> void {
    float aspect = (float)width / (float)height;
    glBindBuffer(GL_UNIFORM_BUFFER, this->gl.globals_ubo);
    glBufferSubData(GL_UNIFORM_BUFFER, 0, sizeof(float), &aspect);
};

auto Renderer::set_scene(const vxng::scene::Scene *scene) -> void {
    throw not_implemented_error();
};

auto Renderer::set_active_camera(const vxng::camera::Camera *camera) -> void {
    this->active_camera = camera;
    // bind this camera's ubo to our bind point
    glBindBufferBase(GL_UNIFORM_BUFFER, UNIF_BIND_CAMERA, camera->get_ubo());
}

auto Renderer::render() const -> void {
    glBindVertexArray(this->gl.vao);
    glUseProgram(this->gl.program);

    // don't use any buffers, we just let our vert shader set gl_Position
    glDrawArrays(GL_TRIANGLES, 0, 3);

    glUseProgram(0);
};

} // namespace vxng
