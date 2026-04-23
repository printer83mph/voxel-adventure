#pragma once

#include "tool.h"

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

  private:
    int depth;
};
