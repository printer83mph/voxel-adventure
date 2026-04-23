#include "palette.h"

Palette::Palette() : colors(), current_color_idx(0) {}
Palette::~Palette() {}

auto Palette::init_default_colors() -> void {
    this->colors.clear();
    this->colors.reserve(6 * 6 * 6);
    for (int r = 0; r < 6; ++r) {
        for (int g = 0; g < 6; ++g) {
            for (int b = 0; b < 6; ++b) {
                this->colors.push_back(glm::u8vec4(r, g, b, 5) * (uint8_t)51);
            }
        }
    }
}

auto Palette::add_color(const glm::u8vec4 &color) -> int {
    auto it = std::find(this->colors.begin(), this->colors.end(), color);
    if (it != this->colors.end())
        return std::distance(this->colors.begin(), it);

    this->colors.push_back(color);
    return this->colors.size() - 1;
}

auto Palette::get_current_color() const -> glm::u8vec4 {
    return this->colors[this->current_color_idx];
}

auto Palette::set_current_color(int index) -> glm::u8vec4 {
    this->current_color_idx = index;
    return this->colors[this->current_color_idx];
}

auto Palette::remove_color(int index) -> void {
    this->colors.erase(std::next(this->colors.begin(), index));

    if (this->current_color_idx >= index && this->current_color_idx > 0)
        --this->current_color_idx;
}
