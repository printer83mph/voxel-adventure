#include "paint-brush.h"

#include <imgui.h>
#include <vxng/geometry.h>

#include <cmath>

PaintBrush::PaintBrush()
    : DraggableTool(5.f), mode(Mode::FULL_KERNEL), size(2),
      airbrush_strength(0.025f), brush_kernel(size), rd(), rgen(rd()),
      rdist(0.f, 1.f) {}

PaintBrush::~PaintBrush() {}

auto PaintBrush::get_tool_name() -> const char * { return "Paintbrush"; }

// no ui for this yet
auto PaintBrush::render_ui() -> void {
    if (ImGui::RadioButton("Full Kernel", (int *)&this->mode,
                           (int)Mode::FULL_KERNEL)) {
    }
    ImGui::SameLine();
    if (ImGui::RadioButton("Airbrush", (int *)&this->mode,
                           (int)Mode::AIRBRUSH)) {
    }
    ImGui::Separator();

    if (ImGui::SliderInt("Size", &this->size, 1, 10))
        this->brush_kernel.set_size(this->size);
    if (ImGui::SliderFloat("Airbrush Strength", &this->airbrush_strength,
                           0.001f, 0.025f))
        this->brush_kernel.set_size(this->size);

    this->render_flow_density_ui();
}

auto PaintBrush::handle_mouse_button_event(const SDL_MouseButtonEvent &event,
                                           const EventBundle &bundle) -> void {
    DraggableTool::handle_mouse_button_event(event, bundle);

    // we don't care about mouse up
    if (!event.down)
        return;

    // only care about left click
    if (event.button != SDL_BUTTON_LEFT)
        return;

    auto mouse_ray = bundle.camera->screen_to_ray(bundle.mouse_ndc_coords);
    auto raycast_result = bundle.scene->raycast(mouse_ray);

    // if nothing hit or we're inside something, do nothing
    if (!raycast_result.hit || raycast_result.inside) {
        return;
    }

    glm::vec3 target_pos =
        mouse_ray.origin + raycast_result.t * mouse_ray.direction;
    glm::vec3 interior_target_pos =
        target_pos - raycast_result.normal * 0.00001f;

    // apply paint!
    stamp_paint(interior_target_pos, bundle);
}

auto PaintBrush::handle_mouse_motion_event(const SDL_MouseMotionEvent &event,
                                           const EventBundle &bundle) -> void {
    DraggableTool::handle_mouse_motion_event(event, bundle);

    auto mouse_ray = bundle.camera->screen_to_ray(bundle.mouse_ndc_coords);
    auto raycast_result = bundle.scene->raycast(mouse_ray);

    // set pointer to "cursor" if raycast hit something
    if (raycast_result.hit) {
        bundle.cursors->set_cursor(Cursors::Variant::POINTER);
    } else {
        bundle.cursors->set_cursor(Cursors::Variant::DEFAULT);
    }
}

auto PaintBrush::handle_keyboard_event(const SDL_KeyboardEvent &event,
                                       const EventBundle &bundle) -> void {}

auto PaintBrush::handle_drag_step(int step_idx, int total_steps,
                                  glm::vec2 step_mouse_ndc_coords,
                                  const EventBundle &bundle) -> void {

    bool is_lmb_dragging = this->is_mousebutton_dragging(SDL_BUTTON_LEFT);

    if (is_lmb_dragging) {
        // only update buffers on last drag step
        bool skip_update_buffers = step_idx < total_steps - 1;

        auto mouse_ray = bundle.camera->screen_to_ray(step_mouse_ndc_coords);
        auto raycast_result = bundle.scene->raycast(mouse_ray);
        glm::vec3 target_pos =
            mouse_ray.origin + raycast_result.t * mouse_ray.direction;
        glm::vec3 interior_target_pos =
            target_pos - raycast_result.normal * 0.00001f;

        stamp_paint(interior_target_pos, bundle, skip_update_buffers);
    }
}

auto PaintBrush::stamp_paint(glm::vec3 position, const EventBundle &bundle,
                             bool skip_update_buffers) -> void {
    int max_depth = std::log2(bundle.scene->get_chunk_resolution());
    float voxel_size =
        bundle.scene->get_chunk_scale() / bundle.scene->get_chunk_resolution();

    for (auto &ioffset : this->brush_kernel.get_kernel()) {
        glm::vec3 offset_position = position + glm::vec3(ioffset) * voxel_size;

        if (this->mode == Mode::AIRBRUSH) {
            if (!sample_airbrush_chance(1.f))
                continue;
        }

        auto sample = bundle.scene->sample_position(offset_position);
        if (sample.has_value() && sample.value() != bundle.current_color)
            bundle.scene->set_voxel_filled(max_depth, offset_position,
                                           bundle.current_color, true);
    }

    // set last guy

    switch (this->mode) {
    case Mode::AIRBRUSH: {
        if (sample_airbrush_chance(1.f))
            bundle.scene->set_voxel_filled(max_depth, position,
                                           bundle.current_color, true);
        if (!skip_update_buffers) {
            // ensure we always update buffers if requested
            bundle.scene->force_update_chunk_buffers(position);
        }
        break;
    }
    case Mode::FULL_KERNEL: {
        bundle.scene->set_voxel_filled(
            max_depth, position, bundle.current_color, skip_update_buffers);
        break;
    }
    }
}

auto PaintBrush::sample_airbrush_chance(float factor) -> bool {
    // random value from 0 to 1
    float rand = this->rdist(this->rgen);
    return rand < this->airbrush_strength * factor;
}
