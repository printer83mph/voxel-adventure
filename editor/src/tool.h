#pragma once

#include "cursors.h"

#include <SDL3/SDL.h>
#include <vxng/vxng.h>

/**
 * This abstract class should be implemented by actual tools. See implemented
 * tools in the `src/tools/` directory.
 */
class EditorTool {
  public:
    EditorTool();
    ~EditorTool();

    virtual auto get_tool_name() -> const char * = 0;
    virtual auto render_ui() -> void = 0;

    typedef struct EventBundle {
        glm::vec2 mouse_ndc_coords;
        vxng::scene::Scene *scene;
        vxng::camera::Camera *camera;
        Cursors *cursors;
    } EventBundle;

    virtual auto handle_mouse_button_event(const SDL_MouseButtonEvent &event,
                                           const EventBundle &bundle)
        -> void = 0;
    virtual auto handle_mouse_motion_event(const SDL_MouseMotionEvent &event,
                                           const EventBundle &bundle)
        -> void = 0;
    virtual auto handle_keyboard_event(const SDL_KeyboardEvent &event,
                                       const EventBundle &bundle) -> void = 0;
};
