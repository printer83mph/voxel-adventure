#include "vxng/scene.h"

#include "chunk.h"

#include <memory>

namespace vxng::scene {

Scene::Scene() {}
Scene::~Scene() {}

auto Scene::init_webgpu(wgpu::Device device) -> void {
    auto origin_chunk = std::make_unique<Chunk>(glm::vec3(0), 4.0, 512);
    origin_chunk->init_webgpu(device);

    this->chunks[glm::ivec3(0)] = std::move(origin_chunk);
}

} // namespace vxng::scene
