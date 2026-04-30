#pragma once

#include "tool.h"

#include <vxng/geometry.h>
#include <vxng/scene.h>

class DraggableTool : public EditorTool {
  public:
    DraggableTool();
    ~DraggableTool();

    virtual auto handle_mouse_button_event(const SDL_MouseButtonEvent &event,
                                           const EventBundle &bundle)
        -> void override;
    virtual auto handle_mouse_motion_event(const SDL_MouseMotionEvent &event,
                                           const EventBundle &bundle)
        -> void override;

    virtual auto handle_activate(const EventBundle &bundle) -> void override;
    virtual auto handle_deactivate(const EventBundle &bundle) -> void override;

  protected:
    float flow_density;

    // consumable by implementations
    auto render_flow_density_ui() -> void;
    auto is_mousebutton_dragging(int sdl_mousebutton) -> bool;
    auto cancel_dragging() -> void;

  private:
    // for tracking drags
    uint32_t drag_buttonmask;
    glm::vec2 last_mouse_ndc_coords;

    // to be overridden by implementations
    virtual auto handle_drag_step(int step_idx, int total_steps,
                                  glm::vec2 step_ndc_mouse_coords) -> void = 0;
};
