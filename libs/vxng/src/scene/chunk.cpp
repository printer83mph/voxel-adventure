#include "chunk.h"

#include <vector>

namespace vxng::scene {

Chunk::Chunk(glm::vec3 pos, float scale, int resolution)
    : position(pos), scale(scale), resolution(resolution) {};
Chunk::~Chunk() {
    if (!this->wgpu.initialized)
        return;

    this->wgpu.octree_ssbo.Destroy();
    this->wgpu.vxdata_ssbo.Destroy();
};

auto Chunk::init_webgpu(wgpu::Device device) -> void {
    wgpu::Buffer octree_ssbo;
    {
        wgpu::BufferDescriptor desc;
        desc.label = "Octree nodes storage buffer";
        desc.size = sizeof(uint32_t) * 3; // one GPUOctreeNode for now
        desc.usage = wgpu::BufferUsage::Storage | wgpu::BufferUsage::CopyDst;
        octree_ssbo = device.CreateBuffer(&desc);
    }

    wgpu::Buffer vxdata_ssbo;
    {
        wgpu::BufferDescriptor desc;
        desc.label = "Voxel data storage buffer";
        desc.size = sizeof(uint32_t); // one GPUVoxelData for now
        desc.usage = wgpu::BufferUsage::Storage | wgpu::BufferUsage::CopyDst;
        vxdata_ssbo = device.CreateBuffer(&desc);
    }

    this->wgpu.initialized = true;
    this->wgpu.device = device;
    this->wgpu.octree_ssbo = octree_ssbo;
    this->wgpu.vxdata_ssbo = vxdata_ssbo;

    update_buffers();
}

auto Chunk::get_octree_buffer() const -> wgpu::Buffer {
    return this->wgpu.octree_ssbo.Get();
}

auto Chunk::get_voxel_data_buffer() const -> wgpu::Buffer {
    return this->wgpu.vxdata_ssbo.Get();
}

typedef struct GPUOctreeNode {
    uint32_t child_mask;      // leaf node if this is empty
    uint32_t first_child_idx; // we can find subsequent child indices
                              // (contiguous) using bitCount(child_mask)
    uint32_t voxel_data_idx;
} GPUOctreeNode;

typedef struct GPUVoxelData {
    uint32_t color_packed;
} GPUVoxelData;

auto Chunk::update_buffers() -> void {
    std::vector<GPUOctreeNode> octree_nodes;
    std::vector<GPUVoxelData> voxel_datas;

    // TODO: implement buffer computation from structure
    // for now: just render one solid cube
    GPUOctreeNode root;
    root.child_mask = 0;
    root.first_child_idx = 0;
    root.voxel_data_idx = 0;
    octree_nodes.push_back(root);

    GPUVoxelData root_data;
    root_data.color_packed =
        (255u << 0) | (0u << 8) | (0u << 16) | (255u << 24); // RGBA red
    voxel_datas.push_back(root_data);

    wgpu::Queue queue = this->wgpu.device.GetQueue();

    queue.WriteBuffer(this->wgpu.octree_ssbo, 0, octree_nodes.data(),
                      sizeof(GPUOctreeNode) * octree_nodes.size());
    queue.WriteBuffer(this->wgpu.vxdata_ssbo, 0, voxel_datas.data(),
                      sizeof(GPUVoxelData) * voxel_datas.size());
}

} // namespace vxng::scene
