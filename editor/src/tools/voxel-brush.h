#pragma once

#include "tool.h"

class VoxelBrush : public EditorTool {
  public:
    VoxelBrush();
    ~VoxelBrush();

    auto get_tool_name() -> const char * override;
    auto render_ui() -> void override;

    auto handle_mouse_button_event(const MouseButtonEventBundle &bundle)
        -> void override;
    auto handle_mouse_motion_event(const MouseMotionEventBundle &bundle)
        -> void override;
    auto handle_keyboard_event(const KeyboardEventBundle &bundle)
        -> void override;
};
