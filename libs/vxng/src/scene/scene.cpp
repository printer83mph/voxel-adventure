#include "vxng/scene.h"

#include "chunk.h"

#include <memory>

#define CHUNK_SCALE 4.0
#define CHUNK_RESOLUTION 512

namespace vxng::scene {

Scene::Scene() {}
Scene::~Scene() {}

auto Scene::init_webgpu(wgpu::Device device) -> void {
    auto origin_chunk =
        std::make_unique<Chunk>(glm::vec3(0), CHUNK_SCALE, CHUNK_RESOLUTION);
    origin_chunk->init_webgpu(device);

    this->chunks[glm::ivec3(0)] = std::move(origin_chunk);
}

auto Scene::get_chunks() const
    -> const std::unordered_map<glm::ivec3, std::unique_ptr<Chunk>> & {
    return this->chunks;
}

} // namespace vxng::scene
