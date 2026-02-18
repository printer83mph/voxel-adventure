#pragma once

// #include "vxng/orbit-camera.h"
// #include "vxng/renderer.h"

#include <SDL3/SDL_events.h>
#include <SDL3/SDL_video.h>
#include <webgpu/webgpu.hpp>

class Editor {
  public:
    Editor();
    ~Editor();

    auto init() -> int;
    auto loop() -> void;

  private:
    SDL_Window *sdl_window;
    struct {
        wgpu::Instance instance;
        wgpu::Surface surface;
    } wgpu;

    // vxng::Renderer renderer;
    // vxng::camera::OrbitCamera viewport_camera;

    auto handle_resize(int width, int height) -> void;
    auto handle_mouse_motion(SDL_MouseMotionEvent event) -> void;
};
