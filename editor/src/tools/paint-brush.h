#pragma once

#include "tools/draggable-tool.h"
#include "tools/shared/brush-kernel.h"

#include <vxng/geometry.h>
#include <vxng/scene.h>

class PaintBrush : public DraggableTool {
  public:
    PaintBrush();
    ~PaintBrush();

    auto get_tool_name() -> const char * override;
    auto render_ui() -> void override;

    auto handle_mouse_button_event(const SDL_MouseButtonEvent &event,
                                   const EventBundle &bundle) -> void override;
    auto handle_mouse_motion_event(const SDL_MouseMotionEvent &event,
                                   const EventBundle &bundle) -> void override;
    auto handle_keyboard_event(const SDL_KeyboardEvent &event,
                               const EventBundle &bundle) -> void override;

  private:
    // params
    int size;

    // stamping
    BrushKernel brush_kernel;

    auto handle_drag_step(int step_idx, int total_steps,
                          glm::vec2 step_mouse_ndc_coords,
                          const EventBundle &bundle) -> void override;

    auto stamp_paint(glm::vec3 position, const EventBundle &bundle,
                     bool skip_update_buffers = false) -> void;
};
