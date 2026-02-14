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
    vec2 ndcCoords = texcoords * 2.0 - 1.0;

    // compute ray direction in view space
    float aspectRatio = 4.0 / 3.0; // TODO: pass as uniform for resizing
    float tanHalfFov = tan(fovYRad * 0.5);
    vec3 rayDirView = normalize(vec3(
        ndcCoords.x * aspectRatio * tanHalfFov,
        ndcCoords.y * tanHalfFov,
        -1.0
    ));

    // transform to world space
    vec3 rayDirWorld = mat3(invViewMat) * rayDirView;
    vec3 rayOrigin = invViewMat[3].xyz;

    FragColor = vec4(rayDirWorld, 1.0);
}
)glsl";

} // namespace vxng::shaders
