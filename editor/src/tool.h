#pragma once

#include "cursors.h"

#include <SDL3/SDL.h>
#include <vxng/vxng.h>

class EditorTool {
  public:
    EditorTool();
    ~EditorTool();

    virtual auto get_tool_name() -> const char * = 0;
    virtual auto render_ui() -> void = 0;

    typedef struct KeyboardEventBundle {
        const SDL_KeyboardEvent *event;
        glm::vec2 mouse_ndc_coords;
        vxng::scene::Scene *scene;
        vxng::camera::Camera *camera;
        Cursors *cursors;
    } KeyboardEventBundle;

    typedef struct MouseButtonEventBundle {
        const SDL_MouseButtonEvent *event;
        glm::vec2 mouse_ndc_coords;
        vxng::scene::Scene *scene;
        vxng::camera::Camera *camera;
        Cursors *cursors;
    } MouseButtonEventBundle;

    typedef struct MouseMotionEventBundle {
        const SDL_MouseMotionEvent *event;
        glm::vec2 mouse_ndc_coords;
        vxng::scene::Scene *scene;
        vxng::camera::Camera *camera;
        Cursors *cursors;
    } MouseMotionEventBundle;

    virtual auto handle_mouse_button_event(const MouseButtonEventBundle &bundle)
        -> void = 0;
    virtual auto handle_mouse_motion_event(const MouseMotionEventBundle &bundle)
        -> void = 0;
    virtual auto handle_keyboard_event(const KeyboardEventBundle &bundle)
        -> void = 0;
};
