#include "vxng/renderer.h"

#include "util.h"

#include <array>
#include <glm/glm.hpp>
#include <webgpu/webgpu_cpp.h>

#include <iostream>
#include <vector>

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
        globals_desc.usage = wgpu::BufferUsage::Uniform;
        globals_buffer = device->CreateBuffer(&globals_desc);

        wgpu::BufferDescriptor camera_desc;
        camera_desc.label = "Camera uniform buffer";
        camera_desc.size = 144;
        camera_desc.usage = wgpu::BufferUsage::Uniform;
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
        camera_entry.binding = 0;
        camera_entry.buffer = camera_buffer;
        camera_entry.offset = 0;
        camera_entry.size = 144;

        wgpu::BindGroupDescriptor bg_desc;
        bg_desc.entryCount = 2;
        bg_desc.entries = &bg_entries[0];
        bind_group = device->CreateBindGroup(&bg_desc);
    }

    // create render pipeline!
    // TODO

    /*
        // OLD OPENGL CODE

        // setup render pipeline
        this->wgpu.initialized = true;
        this->wgpu.device = device;
        this->wgpu.globals_uniforms_buffer = globals_buffer;
        this->wgpu.camera_uniforms_buffer = globals_buffer;

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
            GLuint globals_block = glGetUniformBlockIndex(gl_program,
       "Globals"); GLuint camera_block = glGetUniformBlockIndex(gl_program,
       "Camera"); glUniformBlockBinding(gl_program, globals_block,
       UNIF_BIND_GLOBALS); glUniformBlockBinding(gl_program, camera_block,
       UNIF_BIND_CAMERA);
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
    */

    return true;
}

auto Renderer::resize(int width, int height) -> void {
    float aspect = (float)width / (float)height;
    glBindBuffer(GL_UNIFORM_BUFFER, this->gl.globals_ubo);
    glBufferSubData(GL_UNIFORM_BUFFER, 0, sizeof(float), &aspect);
    glBindBuffer(GL_UNIFORM_BUFFER, 0);
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
