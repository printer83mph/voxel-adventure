#include "chunk.h"
#include "webgpu/webgpu_cpp.h"

#include <vector>

namespace vxng::scene {

// static stuffs
wgpu::BindGroupLayout Chunk::bindgroup_layout = nullptr;
bool Chunk::bindgroup_layout_created = false;

Chunk::Chunk(glm::vec3 pos, float scale, int resolution)
    : position(pos), scale(scale), resolution(resolution) {};
Chunk::~Chunk() {
    if (!this->wgpu.initialized)
        return;

    this->wgpu.octree_buffer.Destroy();
    this->wgpu.vxdata_buffer.Destroy();
};

auto Chunk::init_webgpu(wgpu::Device device) -> void {
    wgpu::Buffer octree_buffer;
    {
        wgpu::BufferDescriptor desc;
        desc.label = "Octree nodes storage buffer";
        desc.size = sizeof(uint32_t) * 3; // one GPUOctreeNode for now
        desc.usage = wgpu::BufferUsage::Storage | wgpu::BufferUsage::CopyDst;
        octree_buffer = device.CreateBuffer(&desc);
    }

    wgpu::Buffer vxdata_buffer;
    {
        wgpu::BufferDescriptor desc;
        desc.label = "Voxel data storage buffer";
        desc.size = sizeof(uint32_t); // one GPUVoxelData for now
        desc.usage = wgpu::BufferUsage::Storage | wgpu::BufferUsage::CopyDst;
        vxdata_buffer = device.CreateBuffer(&desc);
    }

    // run static BGL setup (only once)
    if (!Chunk::bindgroup_layout_created)
        Chunk::create_bindgroup_layout(device);

    wgpu::BindGroup bindgroup;
    {
        wgpu::BindGroupEntry entries[2];

        auto &octree_entry = entries[0];
        octree_entry.binding = 0;
        octree_entry.buffer = octree_buffer;
        octree_entry.offset = 0;
        octree_entry.size = sizeof(uint32_t) * 3;

        auto &vxdata_entry = entries[1];
        vxdata_entry.binding = 1;
        vxdata_entry.buffer = vxdata_buffer;
        vxdata_entry.offset = 0;
        vxdata_entry.size = sizeof(uint32_t);

        wgpu::BindGroupDescriptor bg_desc;
        bg_desc.label = "Chunk data bind group";
        bg_desc.layout = bindgroup_layout;
        bg_desc.entryCount = 2;
        bg_desc.entries = &entries[0];
        bindgroup = device.CreateBindGroup(&bg_desc);
    }

    this->wgpu.initialized = true;
    this->wgpu.device = device;
    this->wgpu.octree_buffer = octree_buffer;
    this->wgpu.vxdata_buffer = vxdata_buffer;
    this->wgpu.bindgroup = bindgroup;

    // get starting data to the gpu!
    update_buffers();
}

auto Chunk::get_bindgroup() const -> wgpu::BindGroup {
    return this->wgpu.bindgroup;
}

auto Chunk::create_bindgroup_layout(wgpu::Device device) -> void {
    wgpu::BindGroupLayoutEntry bgl_entries[2];

    auto &octree_entry = bgl_entries[0];
    octree_entry.binding = 0;
    octree_entry.visibility = wgpu::ShaderStage::Fragment;
    octree_entry.buffer.type = wgpu::BufferBindingType::ReadOnlyStorage;
    octree_entry.buffer.minBindingSize = sizeof(uint32_t) * 3;

    auto &vxdata_entry = bgl_entries[1];
    vxdata_entry.binding = 1;
    vxdata_entry.visibility = wgpu::ShaderStage::Fragment;
    vxdata_entry.buffer.type = wgpu::BufferBindingType::ReadOnlyStorage;
    vxdata_entry.buffer.minBindingSize = sizeof(uint32_t);

    wgpu::BindGroupLayoutDescriptor bgl_descriptor = {};
    bgl_descriptor.label = "Chunk data bind group layout";
    bgl_descriptor.entryCount = 2;
    bgl_descriptor.entries = &bgl_entries[0];

    bindgroup_layout = device.CreateBindGroupLayout(&bgl_descriptor);
    bindgroup_layout_created = true;
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
        (255u << 0) | (0u << 8) | (255u << 16) | (255u << 24); // RGBA magenta
    voxel_datas.push_back(root_data);

    wgpu::Queue queue = this->wgpu.device.GetQueue();

    queue.WriteBuffer(this->wgpu.octree_buffer, 0, octree_nodes.data(),
                      sizeof(GPUOctreeNode) * octree_nodes.size());
    queue.WriteBuffer(this->wgpu.vxdata_buffer, 0, voxel_datas.data(),
                      sizeof(GPUVoxelData) * voxel_datas.size());
}

} // namespace vxng::scene
