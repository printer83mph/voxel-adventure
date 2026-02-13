#include "shaders.h"

namespace vxng::shaders {

const std::string FULLSCREEN_VERT = R"glsl(
#version 410 core

struct CameraUniforms {
    mat4 model;
    mat4 invModel;
    float fovy_rad;
};

// TODO: use camera uniforms
// uniform CameraUniforms camera;

out vec2 texcoords;

void main() {
    vec2 vertices[3]=vec2[3](vec2(-1,-1), vec2(3,-1), vec2(-1, 3));
    gl_Position = vec4(vertices[gl_VertexID],0,1);
    texcoords = 0.5 * gl_Position.xy + vec2(0.5);
}
)glsl";

} // namespace vxng::shaders
