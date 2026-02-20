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

    // Transform to world space
    let invViewMat3 = mat3x3<f32>(
        camera.invViewMat[0].xyz,
        camera.invViewMat[1].xyz,
        camera.invViewMat[2].xyz
    );
    let rayDirWorld = invViewMat3 * rayDirView;
    let rayOrigin = camera.invViewMat[3].xyz;

    return vec4<f32>(rayDirWorld, 1.0);
}
)wgsl";

} // namespace vxng::shaders
