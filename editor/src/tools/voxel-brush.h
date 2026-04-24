#pragma once

#include "tool.h"

#include <vxng/geometry.h>

#include <unordered_map>

class VoxelBrush : public EditorTool {
  public:
    VoxelBrush();
    ~VoxelBrush();

    auto get_tool_name() -> const char * override;
    auto render_ui() -> void override;

    auto handle_mouse_button_event(const SDL_MouseButtonEvent &event,
                                   const EventBundle &bundle) -> void override;
    auto handle_mouse_motion_event(const SDL_MouseMotionEvent &event,
                                   const EventBundle &bundle) -> void override;
    auto handle_keyboard_event(const SDL_KeyboardEvent &event,
                               const EventBundle &bundle) -> void override;

    typedef enum Mode {
        AXIS_ALIGNED,
        CAMERA_PLANE,
    } Mode;

  private:
    Mode current_mode;
    int size;
    int depth;
    float flow_density;

    // for tracking drags
    vxng::geometry::Ray plane_normal;
    glm::vec2 last_mouse_ndc_coords;

    // for brush sizing
    auto compute_brush_kernel() -> void;
    std::unordered_map<glm::uvec3, bool> abs_brush_kernel;
};
