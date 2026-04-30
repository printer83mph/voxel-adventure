#pragma once

#include <glm/glm.hpp>
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/hash.hpp>

#include <unordered_set>

class BrushKernel {
  public:
    BrushKernel(int size);
    ~BrushKernel();

    auto set_size(int size) -> void;
    auto get_size() const -> int;
    auto sample(glm::ivec3 pos) const -> bool;
    auto get_kernel() const -> const std::unordered_set<glm::ivec3> &;

  private:
    int size;
    std::unordered_set<glm::ivec3> kernel;

    auto regenerate_kernel() -> void;
};
