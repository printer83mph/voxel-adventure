#pragma once

#include "vxng/vxng.h"

#include <SDL3/SDL.h>
#include <imgui.h>
#include <webgpu/webgpu_cpp.h>

class Editor {
  public:
    Editor();
    ~Editor();

    auto init() -> int;
    auto run() -> void;

  private:
    ImGuiContext *imgui_context;
    SDL_Window *sdl_window;
    struct {
        wgpu::Device device;
        wgpu::Queue queue;
        wgpu::Surface surface;
        wgpu::TextureFormat preferred_format;
    } wgpu;

    vxng::Renderer renderer;
    vxng::camera::OrbitCamera viewport_camera;
    vxng::scene::Scene scene;

    auto draw_to_surface() -> void;
    auto get_next_surface_texture_view() -> wgpu::TextureView;
    auto get_surface_configuration(int width, int height)
        -> wgpu::SurfaceConfiguration;

    auto poll_events(bool &quit) -> void;
    auto handle_resize(int width, int height) -> void;
    auto handle_key_down(SDL_KeyboardEvent event, bool *quit) -> void;
    auto handle_mouse_motion(SDL_MouseMotionEvent event) -> void;

    struct {
        bool is_active;
        glm::vec3 target, normal;
    } pointer;

    struct {
        SDL_Cursor *normal, *pointer, *orbit_camera, *zoom_camera, *pan_camera;
    } cursors;

    auto update_pointer_target() -> void;
};
