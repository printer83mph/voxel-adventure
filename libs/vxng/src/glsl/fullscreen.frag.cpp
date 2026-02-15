#include "shaders.h"

namespace vxng::shaders {

const std::string FULLSCREEN_FRAG = R"glsl(
#version 410 core
#extension GL_ARB_shader_storage_buffer_object : require

struct OctreeNode {
    uint childMask;
    uint firstChildIdx;
    uint voxelDataIdx;
};

struct VoxelData {
    uint colorPacked;
};

layout (std140) uniform Globals
{
    float aspectRatio;
};
layout (std140) uniform Camera
{
    mat4 viewMat;
    mat4 invViewMat;
    float fovYRad;
};

layout (std430) readonly buffer OctreeNodes
{
    OctreeNode octreeNodes[];
};
layout (std430) readonly buffer VoxelDatas
{
    VoxelData voxelData[];
};

in vec2 texcoords;
out vec4 FragColor;

void main() {
    vec2 ndcCoords = texcoords * 2.0 - 1.0;

    // compute ray direction in view space
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
