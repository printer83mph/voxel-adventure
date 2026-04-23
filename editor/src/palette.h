#pragma once

#include <glm/glm.hpp>

#include <vector>

class Palette {
  public:
    Palette();
    ~Palette();

    auto init_default_colors() -> void;

    /** Returns new index of the color. If the color already exists, does
     * nothing but still returns index. */
    auto add_color(const glm::u8vec4 &color) -> int;
    auto get_current_color() const -> glm::u8vec4;
    auto set_current_color(int index) -> glm::u8vec4;
    auto remove_color(int index) -> void;

    auto run_imgui() -> void;

  private:
    std::vector<glm::u8vec4> colors;
    int current_color_idx;

    glm::vec4 picker_color;
};
