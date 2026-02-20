#include "chunk.h"
#include "webgpu/webgpu_cpp.h"

#include <vector>

namespace vxng::scene {

// static stuffs
wgpu::BindGroupLayout Chunk::bindgroup_layout = nullptr;
bool Chunk::bindgroup_layout_created = false;

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

Chunk::Chunk(glm::vec3 pos, float scale, int resolution)
    : position(pos), scale(scale), resolution(resolution) {};
Chunk::~Chunk() {
    if (!this->wgpu.initialized)
        return;

    this->wgpu.octree_buffer.Destroy();
    this->wgpu.vxdata_buffer.Destroy();
    this->wgpu.metadata_buffer.Destroy();
};

auto Chunk::init_webgpu(wgpu::Device device) -> void {
    this->wgpu.initialized = true;
    this->wgpu.device = device;

    // get starting data to the gpu!
    update_buffers();
}

auto Chunk::get_bindgroup() const -> wgpu::BindGroup {
    return this->wgpu.bindgroup;
}

auto Chunk::create_bindgroup_layout(wgpu::Device device) -> void {
    wgpu::BindGroupLayoutEntry bgl_entries[3];

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

    auto &metadata_entry = bgl_entries[2];
    metadata_entry.binding = 2;
    metadata_entry.visibility = wgpu::ShaderStage::Fragment;
    metadata_entry.buffer.type = wgpu::BufferBindingType::Uniform;
    metadata_entry.buffer.minBindingSize = sizeof(GPUChunkMetadata);

    wgpu::BindGroupLayoutDescriptor bgl_descriptor = {};
    bgl_descriptor.label = "Chunk data bind group layout";
    bgl_descriptor.entryCount = 3;
    bgl_descriptor.entries = &bgl_entries[0];

    bindgroup_layout = device.CreateBindGroupLayout(&bgl_descriptor);
}

auto Chunk::get_bindgroup_layout(wgpu::Device device) -> wgpu::BindGroupLayout {
    if (!bindgroup_layout_created) {
        create_bindgroup_layout(device);
        bindgroup_layout_created = true;
    }

    return bindgroup_layout;
}

auto Chunk::update_buffers() -> void {
    std::vector<GPUOctreeNode> octree_nodes;
    std::vector<GPUVoxelData> voxel_datas;

    // TODO: implement buffer computation from structure
    // for now: just render two leaves
    GPUOctreeNode root;
    root.child_mask = (1u << 7) | 1u;
    root.first_child_idx = 1;
    root.voxel_data_idx = 0;
    octree_nodes.push_back(root);

    GPUOctreeNode firstChild;
    firstChild.child_mask = 0;
    firstChild.first_child_idx = 0;
    firstChild.voxel_data_idx = 0;
    octree_nodes.push_back(firstChild);

    GPUOctreeNode secondChild;
    firstChild.child_mask = 0;
    firstChild.first_child_idx = 0;
    firstChild.voxel_data_idx = 1;
    octree_nodes.push_back(secondChild);

    GPUVoxelData first_data;
    first_data.color_packed =
        (255u << 0) | (0u << 8) | (255u << 16) | (255u << 24); // RGBA magenta
    voxel_datas.push_back(first_data);

    GPUVoxelData second_data;
    second_data.color_packed =
        (0u << 0) | (255u << 8) | (0u << 16) | (255u << 24); // RGBA green
    voxel_datas.push_back(second_data);

    // destroy old buffers (if they exist)
    if (this->wgpu.octree_buffer) {
        this->wgpu.octree_buffer.Destroy();
    }
    if (this->wgpu.vxdata_buffer) {
        this->wgpu.vxdata_buffer.Destroy();
    }
    if (this->wgpu.metadata_buffer) {
        this->wgpu.metadata_buffer.Destroy();
    }

    auto device = this->wgpu.device;
    auto octree_size = sizeof(GPUOctreeNode) * octree_nodes.size();
    auto vxdata_size = sizeof(GPUVoxelData) * voxel_datas.size();

    // new buffers time!
    {
        wgpu::BufferDescriptor desc;
        desc.label = "Octree nodes storage buffer";
        desc.size = octree_size;
        desc.usage = wgpu::BufferUsage::Storage | wgpu::BufferUsage::CopyDst;
        this->wgpu.octree_buffer = device.CreateBuffer(&desc);
    }
    {
        wgpu::BufferDescriptor desc;
        desc.label = "Voxel data storage buffer";
        desc.size = vxdata_size;
        desc.usage = wgpu::BufferUsage::Storage | wgpu::BufferUsage::CopyDst;
        this->wgpu.vxdata_buffer = device.CreateBuffer(&desc);
    }
    {
        wgpu::BufferDescriptor desc;
        desc.label = "Chunk metadata uniform buffer";
        desc.size = sizeof(GPUChunkMetadata);
        desc.usage = wgpu::BufferUsage::Uniform | wgpu::BufferUsage::CopyDst;
        this->wgpu.metadata_buffer = device.CreateBuffer(&desc);
    }

    // send it over to the gpu
    wgpu::Queue queue = device.GetQueue();
    queue.WriteBuffer(this->wgpu.octree_buffer, 0, octree_nodes.data(),
                      octree_size);
    queue.WriteBuffer(this->wgpu.vxdata_buffer, 0, voxel_datas.data(),
                      vxdata_size);

    GPUChunkMetadata metadata;
    metadata.position[0] = this->position.x;
    metadata.position[1] = this->position.y;
    metadata.position[2] = this->position.z;
    metadata.size = this->scale;
    queue.WriteBuffer(this->wgpu.metadata_buffer, 0, &metadata,
                      sizeof(GPUChunkMetadata));

    // bind group (re)creation!
    {
        wgpu::BindGroupEntry entries[3];

        auto &octree_entry = entries[0];
        octree_entry.binding = 0;
        octree_entry.buffer = this->wgpu.octree_buffer;
        octree_entry.offset = 0;
        octree_entry.size = octree_size;

        auto &vxdata_entry = entries[1];
        vxdata_entry.binding = 1;
        vxdata_entry.buffer = this->wgpu.vxdata_buffer;
        vxdata_entry.offset = 0;
        vxdata_entry.size = vxdata_size;

        auto &metadata_entry = entries[2];
        metadata_entry.binding = 2;
        metadata_entry.buffer = this->wgpu.metadata_buffer;
        metadata_entry.offset = 0;
        metadata_entry.size = sizeof(GPUChunkMetadata);

        wgpu::BindGroupDescriptor bg_desc;
        bg_desc.label = "Chunk data bind group";
        bg_desc.layout = get_bindgroup_layout(device);
        bg_desc.entryCount = 3;
        bg_desc.entries = &entries[0];
        this->wgpu.bindgroup = device.CreateBindGroup(&bg_desc);
    }
}

} // namespace vxng::scene
