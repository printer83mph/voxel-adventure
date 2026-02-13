#include "shaders.h"

namespace vxng::shaders {

const std::string FULLSCREEN_FRAG = R"glsl(
#version 410 core

layout (std140) uniform Camera
{
    mat4 viewMat;
    mat4 invViewMat;
    float fovYRad;
};

in vec2 texcoords;
out vec4 FragColor;

void main() {
    float testValue = viewMat[0][1];
    FragColor = vec4(texcoords.x, texcoords.y, testValue, 1.0);
}
)glsl";

} // namespace vxng::shaders
