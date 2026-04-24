#pragma once

#include "vxng/geometry.h"

#include <glm/glm.hpp>
#include <webgpu/webgpu_cpp.h>

#include <array>
#include <memory>
#include <vector>

namespace vxng::scene {

typedef struct VoxelData {
    glm::u8vec4 color; // so much memory eek

    auto operator==(const VoxelData &rhs) -> bool;
    auto operator!=(const VoxelData &rhs) -> bool;
} VoxelData;

typedef struct OctreeNode {
    OctreeNode *parent;
    bool is_leaf;
    VoxelData leaf_data;
    std::array<std::unique_ptr<OctreeNode>, 8> children;

    auto has_children() -> bool;
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

    // --------- Lifecycle ---------

    auto init_webgpu(wgpu::Device device) -> void;

    // --------- Querying ---------

    auto raycast(const geometry::Ray &ray) const -> geometry::RaycastResult;

    // --------- Mutation ---------

    auto set_voxel_filled(int depth, glm::vec3 local_position,
                          glm::u8vec4 color, bool skip_update_buffers = false)
        -> void;
    auto set_voxel_empty(int depth, glm::vec3 local_position,
                         bool skip_update_buffers = false) -> void;
    auto set_voxel_grid_data(const uint8_t *data, glm::ivec3 size,
                             const std::array<glm::u8vec4, 256> &palette,
                             glm::ivec3 offset) -> void;

    // --------- Utility ---------

    auto get_bounds() const -> geometry::AABB;

    /** Sets new position and scale, then updates buffers */
    auto reposition(glm::vec3 pos, float scale) -> void;

    // --------- Rendering ---------

    /** For rendering: we can hook up bindgroup to render this chunk */
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

    /**
     * Dig into the octree, splitting nodes into children if necessary to reach
     * the given depth. Returns a pointer to the node at the given depth.
     *
     * @param local_position  An internal voxel position, in range [-0.5, 0.5]
     * @param depth           The depth to dig, maxing out at `log2(resolution)`
     */
    auto dig_into_tree(glm::vec3 local_position, int depth) -> OctreeNode *;

    /**
     * Creates octree internal nodes everywhere within this chunk, up to the
     * given depth. Should generally be followed by `get_grid_pointers` to get
     * a structured array of pointers to the new octree nodes.
     */
    auto dig_to_depth_everywhere(int depth) -> void;

    /**
     * Gets a structured array of pointers to octree nodes at a certain depth.
     * Should be run after `dig_to_depth_everywhere` for grid-based operations.
     *
     * TODO: maybe combine dig_to_depth_everywhere into this
     */
    auto get_grid_pointers(int depth) -> std::vector<OctreeNode *>;

    /**
     * Recursively relax this chunk's octree, starting from the given node.
     * Will only perform relaxation if the given node either:
     *
     * A) is a leaf node, or
     *
     * B) is an internal node, but the `children` array is all `nullptr`
     *
     * Returns a pointer to the upmost relaxed resulting node, which will be the
     * same as the input if no relaxation was performed.
     */
    auto try_relax_up_from_node(OctreeNode *node) -> OctreeNode *;

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
