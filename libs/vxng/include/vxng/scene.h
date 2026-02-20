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

    auto set_voxel_filled(int depth, glm::vec3 position, glm::u8vec4 color)
        -> void;

    /** Internal method, for renderer to render chunks */
    auto get_chunks() const
        -> const std::unordered_map<glm::ivec3, std::unique_ptr<Chunk>> &;

  private:
    std::unordered_map<glm::ivec3, std::unique_ptr<Chunk>> chunks;

    struct {
        wgpu::Device device;
    } wgpu;
};

} // namespace vxng::scene
