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

struct Ray {
    vec3 origin;
    vec3 direction;
};
struct AABB {
    vec3 bounds_min;
    vec3 bounds_max;
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
// TODO: uniforms for chunk data (pos, scale, etc)

layout (std430) readonly buffer OctreeNodes
{
    OctreeNode octreeNodes[];
};
layout (std430) readonly buffer VoxelDatas
{
    VoxelData voxelDatas[];
};

// from https://github.com/gdbooks/3DCollisions/blob/master/Chapter3/raycast_aabb.md
// spits out t = distance along ray a collision was found, or -1 if none
float raycastAABB(Ray ray, AABB aabb) {
    float t1 = (aabb.bounds_min.x - ray.origin.x) / ray.direction.x;
    float t2 = (aabb.bounds_max.x - ray.origin.x) / ray.direction.x;
    float t3 = (aabb.bounds_min.y - ray.origin.y) / ray.direction.y;
    float t4 = (aabb.bounds_max.y - ray.origin.y) / ray.direction.y;
    float t5 = (aabb.bounds_min.z - ray.origin.z) / ray.direction.z;
    float t6 = (aabb.bounds_max.z - ray.origin.z) / ray.direction.z;

    float tmin = max(max(min(t1, t2), min(t3, t4)), min(t5, t6));
    float tmax = min(min(max(t1, t2), max(t3, t4)), max(t5, t6));

    // if tmax < 0, ray (line) is intersecting AABB, but whole AABB is behing us
    if (tmax < 0.0) {
        return -1.0;
    }

    // if tmin > tmax, ray doesn't intersect AABB
    if (tmin > tmax) {
        return -1.0;
    }
    
    if (tmin < 0.0) {
        return tmax;
    }
    return tmin;
}

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
    Ray viewRay;
    viewRay.direction = mat3(invViewMat) * rayDirView;
    viewRay.origin = invViewMat[3].xyz;

    // TODO: traverse octree instead of just getting root
    // OctreeNode rootNode = octreeNodes[0];

    AABB testAABB;
    testAABB.bounds_min = vec3(-1.0);
    testAABB.bounds_max = vec3(1.0);

    float t = raycastAABB(viewRay, testAABB);

    if (t < 0) {
        FragColor = vec4(viewRay.direction, 1.0);
    } else {
        FragColor = vec4(1.0, 0.0, 0.0, 1.0);
    }
}
)glsl";

} // namespace vxng::shaders
