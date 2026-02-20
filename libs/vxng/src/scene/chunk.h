#pragma once

#include <glm/glm.hpp>
#include <webgpu/webgpu_cpp.h>

#include <array>
#include <memory>

namespace vxng::scene {

typedef struct VoxelData {
    glm::u8vec4 color; // so much memory eek
} VoxelData;

typedef struct OctreeNode {
    bool is_leaf;

    bool leaf_is_solid;
    VoxelData leaf_data;

    std::array<std::unique_ptr<OctreeNode>, 8> children;
} OctreeNode;

typedef struct GPUOctreeNode {
    uint32_t child_mask;      // leaf node if this is empty
    uint32_t first_child_idx; // we can find subsequent child indices
                              // (contiguous) using bitCount(child_mask)
    uint32_t voxel_data_idx;
} GPUOctreeNode;

typedef struct GPUVoxelData {
    uint32_t color_packed;
} GPUVoxelData;

typedef struct GPUChunkMetadata {
    float position[3];
    float size;
} GPUChunkMetadata;

class Chunk {
  public:
    Chunk(glm::vec3 pos, float scale, int resolution);
    ~Chunk();

    auto init_webgpu(wgpu::Device device) -> void;
    auto get_bindgroup() const -> wgpu::BindGroup;

    /** runs create_bindgroup_layout if not bindgroup_layout_created */
    static auto get_bindgroup_layout(wgpu::Device device)
        -> wgpu::BindGroupLayout;

  private:
    static wgpu::BindGroupLayout bindgroup_layout; // shared bindgroup layout
    static bool bindgroup_layout_created;
    /** should only run this once using `bindgroup_layout_created` */
    static auto create_bindgroup_layout(wgpu::Device device) -> void;

    auto update_buffers() -> void;
    auto build_buffer_data(std::vector<GPUOctreeNode> *octree_nodes,
                           std::vector<GPUVoxelData> *voxel_datas) const
        -> void;

    glm::vec3 position;
    float scale;
    int resolution;

    std::unique_ptr<OctreeNode> root_node;

    struct {
        bool initialized;
        wgpu::Device device;
        wgpu::Buffer octree_buffer;
        wgpu::Buffer vxdata_buffer;
        wgpu::Buffer metadata_buffer;
        wgpu::BindGroup bindgroup;
    } wgpu;
};

} // namespace vxng::scene
