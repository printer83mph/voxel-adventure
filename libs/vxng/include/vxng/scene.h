#pragma once

#include "vxng/geometry.h"

#include <glm/glm.hpp>
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/hash.hpp>
#include <webgpu/webgpu_cpp.h>

#include <memory>
#include <unordered_map>

namespace vxng::scene {

class Chunk;

class Scene {
  public:
    Scene(int chunk_resolution, float chunk_scale);
    Scene();
    ~Scene();

    auto init_webgpu(wgpu::Device device) -> void;

    // --------- Queries / Mutation ---------

    auto raycast(const geometry::Ray &ray) const -> geometry::RaycastResult;

    auto set_voxel_filled(int depth, glm::vec3 position, glm::u8vec4 color)
        -> void;
    auto set_voxel_empty(int depth, glm::vec3 position) -> void;

    // --------- Utility ---------

    auto get_chunk_scale() const -> float;
    auto get_chunk_resolution() const -> int;
    auto set_chunk_scale(float new_scale) -> void;

    // --------- Rendering ---------

    /** Internal method, for renderer to render chunks */
    auto get_chunks() const
        -> const std::unordered_map<glm::ivec3, std::unique_ptr<Chunk>> &;

  private:
    float chunk_scale;
    int chunk_resolution;
    std::unordered_map<glm::ivec3, std::unique_ptr<Chunk>> chunks;

    typedef struct ChunkedLocationInfo {
        glm::ivec3 chunk_coord;
        glm::vec3 local_position;
    } LocationInfo;

    /**
     * Given a world position, get the chunk coord at said position as well as
     * the position's chunk-local equivalent position
     */
    auto get_chunked_location_info(glm::vec3 position) const
        -> ChunkedLocationInfo;

    /**
     * Instantiates chunk (and runs webgpu init) at given coords if chunk is not
     * yet instantiated.
     *
     * @return pointer to the new or existing chunk.
     */
    auto touch_chunk(glm::ivec3 chunk_coord) -> Chunk *;

    struct {
        wgpu::Device device;
    } wgpu;
};

} // namespace vxng::scene
