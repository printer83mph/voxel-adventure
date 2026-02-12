#pragma once

#include "SDL3/SDL_video.h"
#include "vxng/renderer.h"

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
};
