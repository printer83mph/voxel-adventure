#pragma once

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
    Scene();
    ~Scene();

    auto init_webgpu(wgpu::Device device) -> void;

  private:
    std::unordered_map<glm::ivec3, std::unique_ptr<Chunk>> chunks;
};

} // namespace vxng::scene
