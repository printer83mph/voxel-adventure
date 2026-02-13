#pragma once

#include "vxng/orbit-camera.h"
#include "vxng/renderer.h"

#include <SDL3/SDL_video.h>

class Editor {
  public:
    Editor();
    ~Editor();

    auto init() -> int;
    auto loop() -> void;

  private:
    SDL_Window *sdl_window;
    SDL_GLContext sdl_gl_context;
    vxng::Renderer renderer;
    vxng::camera::OrbitCamera viewport_camera;

    auto resize(int width, int height) -> void;
};
