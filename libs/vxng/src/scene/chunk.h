#pragma once

#include <GL/glew.h>
#include <glm/glm.hpp>

#include <array>
#include <memory>

namespace vxng::scene {

class Chunk {
  public:
    Chunk(glm::vec3 pos, float scale, int resolution);
    ~Chunk();

    auto init_gl() -> void;

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
        GLuint octree_ssbo;
        GLuint vxdata_ssbo;
    } gl;
};

} // namespace vxng::scene