#include "chunk.h"

#include <stack>
#include <webgpu/webgpu_cpp.h>

#include <cmath>
#include <memory>
#include <queue>
#include <stdexcept>
#include <vector>

namespace vxng::scene {

auto VoxelData::operator==(const VoxelData &rhs) -> bool {
    return this->color == rhs.color;
}
auto VoxelData::operator!=(const VoxelData &rhs) -> bool {
    return !(*this == rhs);
}

auto OctreeNode::has_children() -> bool {
    for (const auto &child : this->children) {
        if (child)
            return true;
    }
    return false;
}

// static stuffs
wgpu::BindGroupLayout Chunk::bindgroup_layout = nullptr;
bool Chunk::bindgroup_layout_created = false;

Chunk::Chunk(glm::vec3 pos, float scale, int resolution)
    : position(pos), scale(scale), resolution(resolution) {
    this->root_node = std::make_unique<OctreeNode>();
    this->root_node->parent = nullptr;
};

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

auto Chunk::get_bounds() const -> geometry::AABB {
    float half_size = scale * 0.5f;
    geometry::AABB bounds;
    bounds.min = position - glm::vec3(half_size);
    bounds.max = position + glm::vec3(half_size);
    return bounds;
}

auto Chunk::is_empty() const -> bool {
    // do DFS, if we find any leaf, it's over bros
    std::stack<OctreeNode *> node_stack = {};

    node_stack.push(this->root_node.get());
    while (!node_stack.empty()) {
        OctreeNode *node = node_stack.top();
        node_stack.pop();

        if (node->is_leaf)
            return false;

        if (!node->has_children())
            continue;

        for (auto &child_uptr : node->children) {
            if (child_uptr)
                node_stack.push(child_uptr.get());
        }
    }

    // no leaves found, we are empty
    return true;
}

auto Chunk::sample_position(glm::vec3 local_position) const
    -> std::optional<glm::u8vec4> {
    const OctreeNode *node = this->root_node.get();

    // create required nodes to specific depth
    for (int trav_depth = 0; (1u << trav_depth) <= this->resolution;
         ++trav_depth) {
        // if solid, just return color
        if (node->is_leaf)
            return node->leaf_data.color;

        // dig into specific child node based on position
        int child_index = ((uint32_t)(local_position.x >= 0) << 0) +
                          ((uint32_t)(local_position.y >= 0) << 1) +
                          ((uint32_t)(local_position.z >= 0) << 2);

        // if we're internal, but child doesn't exist, there's nothing there
        if (!node->children[child_index]) {
            return {};
        }

        // put local position into terms of new node bounds
        local_position = glm::fract((local_position + glm::vec3(0.5f)) * 2.0f) -
                         glm::vec3(0.5f);
        node = node->children[child_index].get();
    }

    return {};
}

auto Chunk::raycast(const geometry::Ray &ray) const -> geometry::RaycastResult {
    // If not leaf but no children, return miss
    if (!root_node->is_leaf && !root_node->has_children()) {
        return geometry::RaycastResult{.hit = false};
    }

    // Compute root AABB from chunk position and scale
    geometry::AABB root_aabb = get_bounds();

    // Check if ray hits root AABB at all
    if (!geometry::ray_aabb_intersect(ray, root_aabb).hit) {
        return geometry::RaycastResult{.hit = false};
    }

    // Stack for iterative DFS traversal
    struct StackEntry {
        const OctreeNode *node;
        geometry::AABB aabb;
    };

    std::vector<StackEntry> stack;
    stack.push_back({root_node.get(), root_aabb});

    geometry::RaycastResult closest_hit{.hit = false};
    float closest_t = 1e30f;

    while (!stack.empty()) {
        StackEntry entry = stack.back();
        stack.pop_back();

        const OctreeNode *node = entry.node;
        const geometry::AABB &aabb = entry.aabb;

        // Check if this is a leaf node (no children)
        if (node->is_leaf) {
            // Raycast against this leaf's AABB
            auto result = geometry::ray_aabb_intersect(ray, aabb);
            if (result.hit) {
                // Check if this is the closest hit so far
                if (result.t >= 0.0f && result.t < closest_t) {
                    closest_t = result.t;

                    // Compute hit point and normal
                    glm::vec3 hit_point = ray.origin + result.t * ray.direction;
                    closest_hit.hit = true;
                    closest_hit.t = result.t;
                    closest_hit.inside = result.inside;
                    closest_hit.normal =
                        geometry::compute_aabb_normal(aabb, hit_point);
                }
            }
        } else {
            // Internal node - push children onto stack in reverse order (for
            // DFS) We want to visit them in order 0-7, so push in reverse 7-0
            bool ray_origin_inside = aabb.contains(ray.origin);
            for (int i = 7; i >= 0; --i) {
                if (node->children[i]) {
                    // Compute child AABB
                    glm::vec3 mid = (aabb.min + aabb.max) * 0.5f;
                    geometry::AABB child_aabb;

                    // Octant bit layout: xyz
                    child_aabb.min.x = (i & 1) ? mid.x : aabb.min.x;
                    child_aabb.min.y = (i & 2) ? mid.y : aabb.min.y;
                    child_aabb.min.z = (i & 4) ? mid.z : aabb.min.z;

                    child_aabb.max.x = (i & 1) ? aabb.max.x : mid.x;
                    child_aabb.max.y = (i & 2) ? aabb.max.y : mid.y;
                    child_aabb.max.z = (i & 4) ? aabb.max.z : mid.z;

                    // Check if ray could hit this child AABB before closest hit
                    auto child_result =
                        geometry::ray_aabb_intersect(ray, child_aabb);
                    if (child_result.hit &&
                        (ray_origin_inside || child_result.t < closest_t)) {
                        stack.push_back({node->children[i].get(), child_aabb});
                    }
                }
            }
        }
    }

    return closest_hit;
}

auto Chunk::set_voxel_filled(int depth, glm::vec3 local_position,
                             glm::u8vec4 color, bool skip_update_buffers)
    -> void {
    // dig first for the node we want to edit
    OctreeNode *node = dig_into_tree(local_position, depth);

    // we have gotten to our desired depth, now just set active node to leaf
    node->is_leaf = true;
    node->leaf_data.color = color;

    // relax upwards if possible
    try_relax_up_from_node(node);

    // and of course update buffers for rendering
    if (!skip_update_buffers)
        update_buffers();
};

auto Chunk::set_voxel_empty(int depth, glm::vec3 local_position,
                            bool skip_update_buffers) -> void {
    // dig first for the node we want to edit
    OctreeNode *node = dig_into_tree(local_position, depth);

    // we have gotten to our desired depth, now just set active node to leaf
    node->is_leaf = false;
    node->children = {};

    // could be optimized by digging to the depth 1 above, then just setting the
    // matching child to nullptr

    // relax upwards if possible
    try_relax_up_from_node(node);

    // and of course update buffers for rendering
    if (!skip_update_buffers)
        update_buffers();
}

auto Chunk::reposition(glm::vec3 pos, float scale) -> void {
    this->position = pos;
    this->scale = scale;

    update_buffers();
}

auto Chunk::collect_colors() const -> std::unordered_set<glm::u8vec4> {
    // do DFS, collect all leaf colors into set
    std::unordered_set<glm::u8vec4> colors = {};
    std::stack<OctreeNode *> node_stack = {};

    node_stack.push(this->root_node.get());
    while (!node_stack.empty()) {
        OctreeNode *node = node_stack.top();
        node_stack.pop();

        if (node->is_leaf) {
            colors.insert(node->leaf_data.color);
            continue;
        }

        if (!node->has_children())
            continue;

        for (auto &child_uptr : node->children) {
            if (child_uptr)
                node_stack.push(child_uptr.get());
        }
    }

    return colors;
}

auto Chunk::set_voxel_grid_data(const uint8_t *data, glm::ivec3 size,
                                const std::array<glm::u8vec4, 256> &palette,
                                glm::ivec3 offset) -> void {
    // assume indices are:
    // x + (y * model->size_x) + (z * model->size_x * model->size_y)

    // very jank, we can optimize much better by digging out octree structure in
    // our region of effect first, and getting a grid-layout vector of
    // OctreeNode pointers/refs to mutate with our data

    for (int x = 0; x < size.x; ++x) {
        int filled = 0;
        for (int y = 0; y < size.y; ++y) {
            for (int z = 0; z < size.z; ++z) {
                int voxel_index = x + (y * size.x) + (z * size.x * size.y);

                auto palette_index = data[voxel_index];
                if (palette_index == 0)
                    continue;

                glm::u8vec4 color = palette[palette_index];

                glm::vec3 coord = glm::ivec3(x, y, z) + offset;
                glm::vec3 local_position =
                    glm::vec3(-0.5) +
                    glm::vec3(1.0f / (float)this->resolution) * coord;

                this->set_voxel_filled(9, local_position, color, true);
                filled++;
            }
        }
    }
    this->update_buffers();
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
    metadata_entry.visibility =
        wgpu::ShaderStage::Vertex | wgpu::ShaderStage::Fragment;
    metadata_entry.buffer.type = wgpu::BufferBindingType::Uniform;

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

auto Chunk::dig_into_tree(glm::vec3 local_position, int depth) -> OctreeNode * {
    OctreeNode *node = this->root_node.get();

    if (depth < 0 || depth > std::log2(this->resolution)) {
        throw std::invalid_argument(
            "Depth must be >= 0 and <= log2(resolution)");
    }

    // create required nodes to specific depth
    for (int trav_depth = 0; trav_depth < depth; ++trav_depth) {
        bool node_is_leaf = node->is_leaf;
        if (node_is_leaf) {
            // if we were filled, then fill all children
            for (int i = 0; i < 8; ++i) {
                node->children[i] = std::make_unique<OctreeNode>();
                auto &child = node->children[i];

                child->parent = node;
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

            child->parent = node;
            child->is_leaf = node_is_leaf;
            child->leaf_data = node->leaf_data;
        }

        // put local position into terms of new node bounds
        local_position = glm::fract((local_position + glm::vec3(0.5f)) * 2.0f) -
                         glm::vec3(0.5f);
        node = node->children[child_index].get();
    }

    return node;
}

auto Chunk::dig_to_depth_everywhere(int depth) -> void {
    // TODO: implement
}

auto Chunk::get_grid_pointers(int depth) -> std::vector<OctreeNode *> {
    // TODO: implement
    return {};
}

auto Chunk::try_relax_up_from_node(OctreeNode *node) -> OctreeNode * {
    OctreeNode *parent = node->parent;
    if (!parent)
        return node;

    if (!node->is_leaf) {
        // internal node: do nothing if still have any children
        if (node->has_children())
            return node;

        // delete our node child
        for (auto &child : parent->children) {
            if (child.get() == node) {
                child = nullptr;
                break;
            }
        }

        // recurse up octree
        return try_relax_up_from_node(parent);
    } else {
        // node is leaf, try to join into parent if all children match

        // end recursion if not all children match
        VoxelData &node_data = node->leaf_data;
        for (auto &child : parent->children) {
            if (!child || child->leaf_data != node_data)
                return node;
        }

        // copy leaf data to parent and reset its children to nullptr
        parent->is_leaf = true;
        parent->leaf_data = node_data;
        parent->children = {};

        // traverse up octree
        return try_relax_up_from_node(parent);
    }
}

} // namespace vxng::scene
