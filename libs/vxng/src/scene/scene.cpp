#include "vxng/scene.h"

#include "chunk.h"

#include <memory>

#define CHUNK_SCALE 4.0f
#define CHUNK_RESOLUTION 512

namespace vxng::scene {

Scene::Scene() {}
Scene::~Scene() {}

auto Scene::init_webgpu(wgpu::Device device) -> void {}

auto Scene::get_chunks() const
    -> const std::unordered_map<glm::ivec3, std::unique_ptr<Chunk>> & {
    return this->chunks;
}

auto Scene::set_voxel_filled(int depth, glm::vec3 position, glm::u8vec4 color)
    -> void {
    glm::ivec3 chunk_coord =
        glm::floor((position + glm::vec3(CHUNK_SCALE * 0.5f)) / CHUNK_SCALE);
    glm::vec3 chunk_origin = (glm::vec3(chunk_coord) * CHUNK_SCALE);
    glm::vec3 local_position = (position - chunk_origin) / CHUNK_SCALE;

    if (!this->chunks[chunk_coord]) {
        this->chunks[chunk_coord] = std::make_unique<Chunk>(
            chunk_origin, CHUNK_SCALE, CHUNK_RESOLUTION);
    }

    this->chunks[chunk_coord]->set_voxel_filled(depth, local_position, color);
}

} // namespace vxng::scene
