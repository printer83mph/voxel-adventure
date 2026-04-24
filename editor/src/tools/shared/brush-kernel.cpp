#include "brush-kernel.h"

BrushKernel::BrushKernel(int size) : size(size), kernel() {
    regenerate_kernel();
}
BrushKernel::~BrushKernel() {}

auto BrushKernel::set_size(int size) -> void {
    this->size = size;
    regenerate_kernel();
}

auto BrushKernel::get_size() const -> int { return this->size; }

auto BrushKernel::sample(glm::ivec3 pos) const -> bool {
    auto kernel_result = this->kernel.find(pos);
    // true if we found coord in the kernel
    return kernel_result != this->kernel.end();
}

auto BrushKernel::get_kernel() const -> const std::unordered_set<glm::ivec3> & {
    return this->kernel;
}

auto BrushKernel::regenerate_kernel() -> void {
    this->kernel.clear();

    int iter_start = -this->size + 1;
    for (int x = iter_start; x < this->size; ++x) {
        for (int y = iter_start; y < this->size; ++y) {
            for (int z = iter_start; z < this->size; ++z) {
                float magnitude = glm::sqrt(x * x + y * y + z * z);
                if (magnitude < (this->size - 0.5f))
                    this->kernel.insert(glm::ivec3(x, y, z));
            }
        }
    }
}
