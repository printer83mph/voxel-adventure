#pragma once

#include <glm/glm.hpp>
#include <webgpu/webgpu_cpp.h>

#include <array>
#include <memory>

namespace vxng::scene {

class Chunk {
  public:
    Chunk(glm::vec3 pos, float scale, int resolution);
    ~Chunk();

    auto init_webgpu(wgpu::Device device) -> void;
    auto get_octree_buffer() const -> wgpu::Buffer;
    auto get_voxel_data_buffer() const -> wgpu::Buffer;

  private:
    auto update_buffers() -> void;

    typedef struct VoxelData {
        glm::u8vec4 color; // so much memory eek
    } VoxelData;

    typedef struct OctreeNode {
        bool is_leaf;

        bool leaf_is_solid;
        VoxelData leaf_data;

        std::array<std::unique_ptr<OctreeNode>, 8> children;
    } OctreeNode;

    glm::vec3 position;
    float scale;
    int resolution;

    std::unique_ptr<OctreeNode> root_node;

    struct {
        bool initialized;
        wgpu::Device device;
        wgpu::Buffer octree_ssbo;
        wgpu::Buffer vxdata_ssbo;
    } wgpu;
};

} // namespace vxng::scene
