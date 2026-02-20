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

struct ChunkMetadata {
    position: vec3<f32>,
    size: f32,
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

struct RaycastHitInfo {
    t: f32,
    normal: vec3<f32>,
}

// same as raycastAABB but also computes the surface normal at the hit point
fn raycastAABBWithNormal(ray: Ray, aabb: AABB) -> RaycastHitInfo {
    var result: RaycastHitInfo;
    result.t = -1.0;
    result.normal = vec3<f32>(0.0);

    let t1 = (aabb.bounds_min.x - ray.origin.x) / ray.direction.x;
    let t2 = (aabb.bounds_max.x - ray.origin.x) / ray.direction.x;
    let t3 = (aabb.bounds_min.y - ray.origin.y) / ray.direction.y;
    let t4 = (aabb.bounds_max.y - ray.origin.y) / ray.direction.y;
    let t5 = (aabb.bounds_min.z - ray.origin.z) / ray.direction.z;
    let t6 = (aabb.bounds_max.z - ray.origin.z) / ray.direction.z;

    let tminX = min(t1, t2);
    let tminY = min(t3, t4);
    let tminZ = min(t5, t6);
    let tmin = max(max(tminX, tminY), tminZ);
    let tmax = min(min(max(t1, t2), max(t3, t4)), max(t5, t6));

    if (tmax < 0.0 || tmin > tmax) {
        return result;
    }

    if (tmin < 0.0) {
        // ray origin is inside the AABB, hit is the exit face
        result.t = tmax;
        let tmaxX = max(t1, t2);
        let tmaxY = max(t3, t4);
        let tmaxZ = max(t5, t6);
        if (tmax == tmaxX) {
            result.normal = vec3<f32>(sign(ray.direction.x), 0.0, 0.0);
        } else if (tmax == tmaxY) {
            result.normal = vec3<f32>(0.0, sign(ray.direction.y), 0.0);
        } else {
            result.normal = vec3<f32>(0.0, 0.0, sign(ray.direction.z));
        }
    } else {
        // normal is on the entry face, pointing against the ray direction
        result.t = tmin;
        if (tmin == tminX) {
            result.normal = vec3<f32>(-sign(ray.direction.x), 0.0, 0.0);
        } else if (tmin == tminY) {
            result.normal = vec3<f32>(0.0, -sign(ray.direction.y), 0.0);
        } else {
            result.normal = vec3<f32>(0.0, 0.0, -sign(ray.direction.z));
        }
    }

    return result;
}

// computes child AABB for an octant (0-7) within a parent AABB
// octant bit layout: bit0 = x, bit1 = y, bit2 = z (0 = low, 1 = high)
fn childAABB(parent: AABB, octant: u32) -> AABB {
    let mid = (parent.bounds_min + parent.bounds_max) * 0.5;
    var child: AABB;
    child.bounds_min = select(parent.bounds_min, mid, vec3<bool>(
        (octant & 1u) != 0u,
        (octant & 2u) != 0u,
        (octant & 4u) != 0u,
    ));
    child.bounds_max = select(mid, parent.bounds_max, vec3<bool>(
        (octant & 1u) != 0u,
        (octant & 2u) != 0u,
        (octant & 4u) != 0u,
    ));
    return child;
}

// unpacks an RGBA8 packed color into a vec4<f32>
fn unpackColor(packed: u32) -> vec4<f32> {
    let r = f32(packed & 0xFFu) / 255.0;
    let g = f32((packed >> 8u) & 0xFFu) / 255.0;
    let b = f32((packed >> 16u) & 0xFFu) / 255.0;
    let a = f32((packed >> 24u) & 0xFFu) / 255.0;
    return vec4<f32>(r, g, b, a);
}

// stack entry for iterative octree traversal
// we gotta store bounds since we're not packing that into voxel data
struct StackEntry {
    nodeIdx: u32,
    boundsMin: vec3<f32>,
    boundsMax: vec3<f32>,
    nextOctant: u32, // next child octant to visit (0-8, 8 = done)
}

const MAX_STACK_DEPTH: u32 = 16u;
const SKY_COLOR = vec4<f32>(0.0);

struct TraversalResult {
    color: vec4<f32>,
    normal: vec3<f32>,
    t: f32,
}

// traverse octree iteratively, returning the color and normal of the closest hit leaf
fn traverseOctree(ray: Ray, rootAABB: AABB) -> TraversalResult {
    var miss: TraversalResult;
    miss.color = SKY_COLOR;
    miss.normal = vec3<f32>(0.0);
    miss.t = -1.0;

    // make sure the ray hits the root AABB at all
    let rootT = raycastAABB(ray, rootAABB);
    if (rootT < 0.0) {
        return miss;
    }

    var stack: array<StackEntry, 16>;
    var stackPtr: i32 = 0;

    // push root onto stack
    stack[0].nodeIdx = 0u;
    stack[0].boundsMin = rootAABB.bounds_min;
    stack[0].boundsMax = rootAABB.bounds_max;
    stack[0].nextOctant = 0u;

    var closestT: f32 = 1e30;
    var closestColor: vec4<f32> = SKY_COLOR;
    var closestNormal: vec3<f32> = vec3<f32>(0.0);

    // DFS traversal
    while (stackPtr >= 0) {
        let depth = stackPtr;
        let node = octreeNodes[stack[depth].nodeIdx];
        var parentBounds: AABB;
        parentBounds.bounds_min = stack[depth].boundsMin;
        parentBounds.bounds_max = stack[depth].boundsMax;

        // we know leaf node if childMask == 0
        if (node.childMask == 0u) {
            let hit = raycastAABBWithNormal(ray, parentBounds);
            if (hit.t >= 0.0 && hit.t < closestT) {
                closestT = hit.t;
                closestNormal = hit.normal;
                closestColor = unpackColor(
                    voxelData[node.voxelDataIdx].colorPacked
                );
            }
            // Pop
            stackPtr -= 1;
            continue;
        }

        // internal node: find next child octant to visit
        var pushed = false;
        for (var o = stack[depth].nextOctant; o < 8u; o += 1u) {
            if ((node.childMask & (1u << o)) == 0u) {
                continue;
            }

            let cBounds = childAABB(parentBounds, o);
            let t = raycastAABB(ray, cBounds);

            // skip any children that are behind the closest known hit
            // super optimized!!!
            if (t < 0.0 || t >= closestT) {
                continue;
            }

            // record where parent should resume when we come back
            stack[depth].nextOctant = o + 1u;

            // compute child's index in the contiguous array:
            // firstChildIdx + number of set bits below bit o
            let maskBelow = node.childMask & ((1u << o) - 1u);
            let childOffset = countOneBits(maskBelow);
            let childIdx = node.firstChildIdx + childOffset;

            // push child
            stackPtr += 1;
            stack[stackPtr].nodeIdx = childIdx;
            stack[stackPtr].boundsMin = cBounds.bounds_min;
            stack[stackPtr].boundsMax = cBounds.bounds_max;
            stack[stackPtr].nextOctant = 0u;
            pushed = true;
            break;
        }

        // no more children to visit, pop this node
        if (!pushed) {
            stackPtr -= 1;
        }
    }

    var result: TraversalResult;
    result.color = closestColor;
    result.normal = closestNormal;
    result.t = closestT;
    return result;
}

@group(0) @binding(0) var<uniform> globals: Globals;
@group(1) @binding(0) var<uniform> camera: Camera;
@group(2) @binding(0) var<storage, read> octreeNodes: array<OctreeNode>;
@group(2) @binding(1) var<storage, read> voxelData: array<VoxelData>;
@group(2) @binding(2) var<uniform> chunkMetadata: ChunkMetadata;

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

    // setup root AABB from chunk metadata
    let halfSize = chunkMetadata.size * 0.5;
    var rootAABB: AABB;
    rootAABB.bounds_min = chunkMetadata.position - vec3<f32>(halfSize);
    rootAABB.bounds_max = chunkMetadata.position + vec3<f32>(halfSize);

    // traverse octree!
    let result = traverseOctree(viewRay, rootAABB);
    if (result.color.a == 0.0) {
        // if no hit, show ray direction as sky
        return vec4<f32>(viewRay.direction, 1.0);
    }

    // simple half lambert lighting
    let lightDir = normalize(vec3<f32>(0.5, 1.0, 0.3));
    let ambient = 0.2;
    let diffuse = max(dot(result.normal, lightDir) * 0.5 + 0.5, 0.0);
    let lighting = ambient + (1.0 - ambient) * diffuse;
    return vec4<f32>(result.color.rgb * lighting, result.color.a);
}
)wgsl";

} // namespace vxng::shaders
