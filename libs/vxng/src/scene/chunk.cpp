#include "chunk.h"

#include <webgpu/webgpu_cpp.h>

#include <memory>
#include <queue>
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

auto Chunk::set_voxel_filled(int depth, glm::vec3 local_position,
                             glm::u8vec4 color) -> void {
    if (!this->root_node) {
        this->root_node = std::make_unique<OctreeNode>();
    }
    auto *node = this->root_node.get();

    // create required nodes to specific depth
    for (int trav_depth = 0; trav_depth < depth; ++trav_depth) {
        bool node_is_leaf = node->is_leaf;
        if (node_is_leaf) {
            // if we were filled, then fill all children
            for (int i = 0; i < 8; ++i) {
                node->children[i] = std::make_unique<OctreeNode>();
                auto &child = node->children[i];

                child->is_leaf = true;
                child->leaf_data = node->leaf_data;
            }
        }
        node->is_leaf = false;

        // dig into specific child node based on position
        int child_index = ((uint32_t)(local_position.x >= 0) << 0) +
                          ((uint32_t)(local_position.y >= 0) << 1) +
                          ((uint32_t)(local_position.z >= 0) << 2);

        // make child node if not exists
        if (!node->children[child_index]) {
            node->children[child_index] = std::make_unique<OctreeNode>();
            auto &child = node->children[child_index];

            child->is_leaf = node_is_leaf;
            child->leaf_data = node->leaf_data;
        }

        // put local position into terms of new node bounds
        local_position = glm::fract((local_position + glm::vec3(0.5f)) * 2.0f) -
                         glm::vec3(0.5f);
        node = node->children[child_index].get();
    }

    // we have gotten to our desired depth, now just set active node to leaf
    node->is_leaf = true;
    node->leaf_data.color = color;

    // and of course update buffers for rendering
    update_buffers();
};

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

    // fill up buffers
    build_buffer_data(&octree_nodes, &voxel_datas);

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

auto Chunk::build_buffer_data(std::vector<GPUOctreeNode> *octree_nodes,
                              std::vector<GPUVoxelData> *voxel_datas) const
    -> void {

    // guarantee at least one voxel data entry (satisfies minBindingSize)
    // it will have zero opacity LOL
    // TODO: this should probably be made more intentionally
    GPUVoxelData empty_voxel{};
    empty_voxel.color_packed = 0;
    voxel_datas->push_back(empty_voxel);

    // if no octree yet, push a single empty leaf node.
    if (!root_node) {
        GPUOctreeNode empty_leaf{};
        empty_leaf.child_mask = 0;
        empty_leaf.first_child_idx = 0;
        empty_leaf.voxel_data_idx = 0;
        octree_nodes->push_back(empty_leaf);
        return;
    }

    // bfs, spit out all children contiguous just how the GPU likes it
    // (first_child_idx + bitcount to get a specific child)
    std::queue<const OctreeNode *> bfs;
    bfs.push(root_node.get());

    while (!bfs.empty()) {
        const OctreeNode *node = bfs.front();
        bfs.pop();

        GPUOctreeNode gpu_node{};
        gpu_node.child_mask = 0;
        gpu_node.first_child_idx = 0;
        gpu_node.voxel_data_idx = 0;

        if (node->is_leaf) {
            // leaf -> child_mask stays 0
            gpu_node.voxel_data_idx =
                static_cast<uint32_t>(voxel_datas->size());

            auto c = node->leaf_data.color;
            GPUVoxelData vdata{};
            vdata.color_packed = (static_cast<uint32_t>(c.r) << 0) |
                                 (static_cast<uint32_t>(c.g) << 8) |
                                 (static_cast<uint32_t>(c.b) << 16) |
                                 (static_cast<uint32_t>(c.a) << 24);
            voxel_datas->push_back(vdata);
        } else {
            // internal node -> make child_mask from non-null children.
            for (int i = 0; i < 8; i++) {
                if (node->children[i]) {
                    gpu_node.child_mask |= (1u << i);
                }
            }

            // children will go after all emitted + queued nodes
            //   index = (emitted) + (queue size) + 1 (this node)
            gpu_node.first_child_idx =
                static_cast<uint32_t>(octree_nodes->size() + bfs.size() + 1);

            // enqueue non-null children in octant order (0-7).
            for (int i = 0; i < 8; i++) {
                if (node->children[i]) {
                    bfs.push(node->children[i].get());
                }
            }
        }

        octree_nodes->push_back(gpu_node);
    }
}

} // namespace vxng::scene
