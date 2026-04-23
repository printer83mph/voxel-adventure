#include "palette.h"

#include <imgui.h>

Palette::Palette() : colors(), current_color_idx(0) {}
Palette::~Palette() {}

auto Palette::init_default_colors() -> void {
    this->colors.clear();
    this->colors.reserve(4 * 4 * 4);
    for (int r = 0; r < 4; ++r) {
        for (int g = 0; g < 4; ++g) {
            for (int b = 0; b < 4; ++b) {
                this->colors.push_back(glm::u8vec4(r, g, b, 3) * (uint8_t)85);
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

auto Palette::run_imgui() -> void {
    auto default_flags =
        ImGuiColorEditFlags_NoPicker | ImGuiColorEditFlags_NoTooltip;
    for (int i = 0; i < this->colors.size(); ++i) {
        ImGui::PushID(i);

        if ((i % 8) != 0)
            ImGui::SameLine(0.0f, ImGui::GetStyle().ItemSpacing.y);

        auto &color = this->colors[i];
        if (ImGui::ColorButton(
                "##palette",
                ImVec4(color.r / 255.f, color.g / 255.f, color.b / 255.f, 1.f),
                current_color_idx == i
                    ? default_flags
                    : default_flags | ImGuiColorEditFlags_NoBorder,
                ImVec2(16, 16))) {
            set_current_color(i);
        }

        ImGui::PopID();
    }
}
