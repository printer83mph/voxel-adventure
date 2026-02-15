#include "chunk.h"

#include <vector>

namespace vxng::scene {

Chunk::Chunk(glm::vec3 pos, float scale, int resolution)
    : position(pos), scale(scale), resolution(resolution) {};
Chunk::~Chunk() = default;

auto Chunk::init_gl() -> void {
    GLuint octree_ssbo, vxdata_ssbo;
    glGenBuffers(1, &octree_ssbo);
    glGenBuffers(1, &vxdata_ssbo);

    this->gl.octree_ssbo = octree_ssbo;
    this->gl.vxdata_ssbo = vxdata_ssbo;
    this->gl.initialized = true;
}

typedef struct GPUOctreeNode {
    unsigned int child_mask;      // leaf node if this is empty
    unsigned int first_child_idx; // we can find subsequent child indices
                                  // (contiguous) using bitCount(child_mask)
    unsigned int voxel_data_idx;
} GPUOctreeNode;

typedef struct GPUVoxelData {
    union {
        uint8_t r, g, b;
        unsigned int value;
    } color_packed;
} GPUVoxelData;

auto Chunk::update_buffers() -> void {
    std::vector<GPUOctreeNode> octree_nodes;
    std::vector<GPUVoxelData> voxel_datas;

    // TODO: implement buffer computation from structure
    // for now: just render one solid cube
    GPUOctreeNode root;
    root.child_mask = 0;
    root.voxel_data_idx = 0;
    octree_nodes.push_back(root);

    GPUVoxelData root_data;
    root_data.color_packed.r = 255;
    root_data.color_packed.g = 0;
    root_data.color_packed.b = 0;
    voxel_datas.push_back(root_data);

    glBindBuffer(GL_SHADER_STORAGE_BUFFER, this->gl.octree_ssbo);
    glBufferData(GL_SHADER_STORAGE_BUFFER,
                 sizeof(GPUOctreeNode) * octree_nodes.size(), &octree_nodes[0],
                 GL_DYNAMIC_DRAW);
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);

    glBindBuffer(GL_SHADER_STORAGE_BUFFER, this->gl.vxdata_ssbo);
    glBufferData(GL_SHADER_STORAGE_BUFFER,
                 sizeof(GPUOctreeNode) * voxel_datas.size(), &voxel_datas[0],
                 GL_DYNAMIC_DRAW);
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);
}

} // namespace vxng::scene