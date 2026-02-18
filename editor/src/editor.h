#pragma once

// #include "vxng/orbit-camera.h"
#include "vxng/renderer.h"

#include <SDL3/SDL.h>
#include <webgpu/webgpu_cpp.h>

class Editor {
  public:
    Editor();
    ~Editor();

    auto init() -> int;
    auto run() -> void;

  private:
    SDL_Window *sdl_window;
    struct {
        wgpu::Device device;
        wgpu::Queue queue;
        wgpu::Surface surface;
    } wgpu;

    vxng::Renderer renderer;
    // vxng::camera::OrbitCamera viewport_camera;

    auto draw_to_surface() -> void;
    auto get_next_surface_texture_view() -> wgpu::TextureView;

    auto poll_events(bool &quit) -> void;
    auto handle_resize(int width, int height) -> void;
    auto handle_mouse_motion(SDL_MouseMotionEvent event) -> void;
};
