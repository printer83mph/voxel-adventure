#include "draggable-tool.h"

#include <imgui.h>
#include <vxng/geometry.h>

DraggableTool::DraggableTool()
    : flow_density(5.f), drag_buttonmask(0u), last_mouse_ndc_coords() {}

DraggableTool::~DraggableTool() {}

auto DraggableTool::render_flow_density_ui() -> void {
    ImGui::SliderFloat("Flow Density", &this->flow_density, 0.f, 20.f);
}

auto DraggableTool::is_mousebutton_dragging(int sdl_mousebutton) -> bool {
    return (bool)(this->drag_buttonmask & SDL_BUTTON_MASK(sdl_mousebutton));
}

auto DraggableTool::cancel_dragging() -> void { this->drag_buttonmask = 0u; }

auto DraggableTool::handle_mouse_button_event(const SDL_MouseButtonEvent &event,
                                              const EventBundle &bundle)
    -> void {
    if (event.down) {
        // start tracking this drag
        this->drag_buttonmask |= SDL_BUTTON_MASK(event.button);
    }
}

auto DraggableTool::handle_mouse_motion_event(const SDL_MouseMotionEvent &event,
                                              const EventBundle &bundle)
    -> void {
    auto mouse_ray = bundle.camera->screen_to_ray(bundle.mouse_ndc_coords);
    auto raycast_result = bundle.scene->raycast(mouse_ray);

    // clear drag state if button not held (all buttons)
    bool is_any_button_held = false;
    for (int button = 1; button <= 5; ++button) {
        int buttonmask = SDL_BUTTON_MASK(button);
        if (!(event.state & buttonmask)) {
            this->drag_buttonmask &= ~buttonmask;
            is_any_button_held = true;
        }
    }

    // check dragging buttonmask
    if (is_any_button_held) {
        float mouse_move_distance =
            ((bundle.mouse_ndc_coords - this->last_mouse_ndc_coords) *
             glm::vec2(bundle.camera->get_aspect_ratio(), 1.f))
                .length();

        int flow_step_count =
            glm::ceil(mouse_move_distance * this->flow_density);

        for (int step = 0; step < flow_step_count; ++step) {
            glm::vec2 lerped_mouse_ndc_coords =
                glm::mix(this->last_mouse_ndc_coords, bundle.mouse_ndc_coords,
                         (float)(step + 1.f) / (float)flow_step_count);

            // pass to implementer
            handle_drag_step(step, flow_step_count, lerped_mouse_ndc_coords,
                             bundle);
        }
    }

    this->last_mouse_ndc_coords = bundle.mouse_ndc_coords;
}

auto DraggableTool::handle_activate(const EventBundle &bundle) -> void {}

auto DraggableTool::handle_deactivate(const EventBundle &bundle) -> void {
    this->drag_buttonmask = 0u;
}
