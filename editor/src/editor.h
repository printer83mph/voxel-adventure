#pragma once

#include "cursors.h"
#include "tool.h"
#include "tools/tools.h"

#include <vxng/vxng.h>

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
    auto run_gui() -> void;
    auto get_next_surface_texture_view() -> wgpu::TextureView;
    auto get_surface_configuration(int width, int height)
        -> wgpu::SurfaceConfiguration;

    auto poll_events(bool &quit) -> void;
    auto handle_resize(int width, int height) -> void;
    auto handle_key_down(const SDL_KeyboardEvent &event, bool *quit) -> void;
    auto handle_key_up(const SDL_KeyboardEvent &event) -> void;
    auto handle_mouse_motion(const SDL_MouseMotionEvent &event) -> void;
    auto handle_mouse_down(const SDL_MouseButtonEvent &event) -> void;
    auto handle_mouse_up(const SDL_MouseButtonEvent &event) -> void;

    Cursors cursors;

    struct {
        VoxelBrush voxel_brush;
    } tools;
    EditorTool *current_tool;

    struct {
        bool show_tools = true;
        bool show_options = true;
    } panels;

    auto get_mouse_ndc_coords() const -> glm::vec2;
};
