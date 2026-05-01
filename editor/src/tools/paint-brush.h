#pragma once

#include "tools/draggable-tool.h"
#include "tools/shared/brush-kernel.h"

#include <vxng/geometry.h>
#include <vxng/scene.h>

#include <random>

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
    typedef enum Mode {
        FULL_KERNEL,
        AIRBRUSH,
    } Mode;

    // params
    Mode mode;
    int size;
    float airbrush_strength;

    // stamping / airbrushing
    BrushKernel brush_kernel;
    std::random_device rd;
    std::mt19937 rgen;
    std::uniform_real_distribution<float> rdist;

    auto handle_drag_step(int step_idx, int total_steps,
                          glm::vec2 step_mouse_ndc_coords,
                          const EventBundle &bundle) -> void override;

    auto stamp_paint(glm::vec3 position, const EventBundle &bundle,
                     bool skip_update_buffers = false) -> void;

    auto sample_airbrush_chance(float factor) -> bool;
};
