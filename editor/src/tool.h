#pragma once

#include <vxng/vxng.h>

#include <SDL3/SDL.h>

typedef struct KeyboardEventBundle {
    const SDL_KeyboardEvent *event;
    glm::vec2 mouse_ndc_coords;
    vxng::scene::Scene *scene;
    vxng::camera::Camera *camera;
} KeyboardEventBundle;

typedef struct MouseButtonEventBundle {
    const SDL_MouseButtonEvent *event;
    glm::vec2 mouse_ndc_coords;
    vxng::scene::Scene *scene;
    vxng::camera::Camera *camera;
} MouseButtonEventBundle;

typedef struct MouseMotionEventBundle {
    const SDL_MouseMotionEvent *event;
    glm::vec2 mouse_ndc_coords;
    vxng::scene::Scene *scene;
    vxng::camera::Camera *camera;
} MouseMotionEventBundle;

class EditorTool {
  public:
    EditorTool();
    ~EditorTool();

    virtual auto get_tool_name() -> const char * = 0;
    virtual auto render_ui() -> void = 0;

    virtual auto handle_mouse_button_event(const MouseButtonEventBundle &bundle)
        -> void = 0;
    virtual auto handle_mouse_motion_event(const MouseMotionEventBundle &bundle)
        -> void = 0;
    virtual auto handle_keyboard_event(const KeyboardEventBundle &bundle)
        -> void = 0;
};
