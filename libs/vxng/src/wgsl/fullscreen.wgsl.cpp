#include "shaders.h"

namespace vxng::shaders {

const std::string FULLSCREEN_WGSL = R"wgsl(
// Uniform buffer for global settings
struct Globals {
    aspectRatio: f32,
}

// Uniform buffer for camera settings
struct Camera {
    viewMat: mat4x4<f32>,
    invViewMat: mat4x4<f32>,
    fovYRad: f32,
}

struct OctreeNode {
    childMask: u32,
    firstChildIdx: u32,
    voxelDataIdx: u32,
}

struct VoxelData {
    colorPacked: u32,
}

struct Ray {
    origin: vec3<f32>,
    direction: vec3<f32>,
}

struct AABB {
    bounds_min: vec3<f32>,
    bounds_max: vec3<f32>,
}

// from https://github.com/gdbooks/3DCollisions/blob/master/Chapter3/raycast_aabb.md
// spits out t = distance along ray a collision was found, or -1 if none
fn raycastAABB(ray: Ray, aabb: AABB) -> f32 {
    let t1 = (aabb.bounds_min.x - ray.origin.x) / ray.direction.x;
    let t2 = (aabb.bounds_max.x - ray.origin.x) / ray.direction.x;
    let t3 = (aabb.bounds_min.y - ray.origin.y) / ray.direction.y;
    let t4 = (aabb.bounds_max.y - ray.origin.y) / ray.direction.y;
    let t5 = (aabb.bounds_min.z - ray.origin.z) / ray.direction.z;
    let t6 = (aabb.bounds_max.z - ray.origin.z) / ray.direction.z;
    let tmin = max(max(min(t1, t2), min(t3, t4)), min(t5, t6));
    let tmax = min(min(max(t1, t2), max(t3, t4)), max(t5, t6));
    // if tmax < 0, ray (line) is intersecting AABB, but whole AABB is behind us
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

@group(0) @binding(0) var<uniform> globals: Globals;
@group(1) @binding(0) var<uniform> camera: Camera;

// Vertex shader output / Fragment shader input
struct VertexOutput {
    @builtin(position) position: vec4<f32>,
    @location(0) texcoords: vec2<f32>,
}

@vertex
fn vs_main(@builtin(vertex_index) vertexIndex: u32) -> VertexOutput {
    // Fullscreen triangle vertices
    var vertices = array<vec2<f32>, 3>(
        vec2<f32>(-1.0, -1.0),
        vec2<f32>(3.0, -1.0),
        vec2<f32>(-1.0, 3.0)
    );

    var output: VertexOutput;
    output.position = vec4<f32>(vertices[vertexIndex], 0.0, 1.0);
    output.texcoords = 0.5 * output.position.xy + vec2<f32>(0.5);
    return output;
}

@fragment
fn fs_main(input: VertexOutput) -> @location(0) vec4<f32> {
    let ndcCoords = input.texcoords * 2.0 - 1.0;

    // return vec4f(input.texcoords, 1.0, 1.0);

    // Compute ray direction in view space
    let tanHalfFov = tan(camera.fovYRad * 0.5);
    let rayDirView = normalize(vec3<f32>(
        ndcCoords.x * globals.aspectRatio * tanHalfFov,
        ndcCoords.y * tanHalfFov,
        -1.0
    ));

    // Build view ray
    let invViewMat3 = mat3x3<f32>(
        camera.invViewMat[0].xyz,
        camera.invViewMat[1].xyz,
        camera.invViewMat[2].xyz
    );
    var viewRay: Ray;
    viewRay.direction = invViewMat3 * rayDirView;
    viewRay.origin = camera.invViewMat[3].xyz;

    // TODO: traverse octree instead of just getting root
    // let rootNode = octreeNodes[0];
    var testAABB: AABB;
    testAABB.bounds_min = vec3<f32>(-1.0);
    testAABB.bounds_max = vec3<f32>(1.0);
    let t = raycastAABB(viewRay, testAABB);
    if (t < 0.0) {
        return vec4<f32>(viewRay.direction, 1.0);
    } else {
        return vec4<f32>(1.0, 0.0, 0.0, 1.0);
    }
}
)wgsl";

} // namespace vxng::shaders
