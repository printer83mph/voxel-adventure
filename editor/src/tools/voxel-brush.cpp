#include "voxel-brush.h"

#include <SDL3/SDL_events.h>
#include <imgui.h>
#include <vxng/geometry.h>

VoxelBrush::VoxelBrush() : depth(9), plane_normal() {}

VoxelBrush::~VoxelBrush() {}

auto VoxelBrush::get_tool_name() -> const char * {
    return "Simple Voxel Brush";
}

// no ui for this yet
auto VoxelBrush::render_ui() -> void {
    ImGui::SliderInt("Depth", &this->depth, 0, 9);
}

auto VoxelBrush::handle_mouse_button_event(const SDL_MouseButtonEvent &event,
                                           const EventBundle &bundle) -> void {
    // we don't care about mouse up
    if (!event.down)
        return;

    // don't do anything if any mods are held
    SDL_Keymod mods = SDL_GetModState();
    if (mods != SDL_KMOD_NONE)
        return;

    auto mouse_ray = bundle.camera->screen_to_ray(bundle.mouse_ndc_coords);
    auto raycast_result = bundle.scene->raycast(mouse_ray);

    // if nothing hit or we're inside something, do nothing
    if (!raycast_result.hit || raycast_result.inside) {
        return;
    }

    glm::vec3 target_pos =
        mouse_ray.origin + raycast_result.t * mouse_ray.direction;

    // set voxel filled!
    switch (event.button) {
    case SDL_BUTTON_LEFT: {
        // go outside of voxel boundary
        glm::vec3 exterior_target_pos =
            target_pos + raycast_result.normal * 0.00001f;

        bundle.scene->set_voxel_filled(this->depth, exterior_target_pos,
                                       bundle.current_color);
        this->plane_normal = vxng::geometry::Ray{
            .origin = exterior_target_pos,
            .direction = raycast_result.normal,
        };
        break;
    }
    case SDL_BUTTON_RIGHT: {
        // go inside voxel boundary
        glm::vec3 interior_target_pos =
            target_pos - raycast_result.normal * 0.00001f;
        bundle.scene->set_voxel_empty(this->depth, interior_target_pos);
        break;
    }
    }
}

auto VoxelBrush::handle_mouse_motion_event(const SDL_MouseMotionEvent &event,
                                           const EventBundle &bundle) -> void {
    auto mouse_ray = bundle.camera->screen_to_ray(bundle.mouse_ndc_coords);
    auto raycast_result = bundle.scene->raycast(mouse_ray);

    // set pointer to "cursor" if raycast hit something
    if (raycast_result.hit) {
        bundle.cursors->set_cursor(Cursors::Variant::POINTER);
    } else {
        bundle.cursors->set_cursor(Cursors::Variant::DEFAULT);
    }

    if (event.state & SDL_BUTTON_LEFT) {
        auto mouse_ray = bundle.camera->screen_to_ray(bundle.mouse_ndc_coords);

        // project mouse ray onto plane defined by plane_normal
        float denom =
            glm::dot(this->plane_normal.direction, mouse_ray.direction);
        if (std::abs(denom) > 1e-6f) {
            float t = glm::dot(this->plane_normal.origin - mouse_ray.origin,
                               this->plane_normal.direction) /
                      denom;
            if (t >= 0.0f) {
                // place voxel if hit
                glm::vec3 intersection =
                    mouse_ray.origin + t * mouse_ray.direction;
                glm::vec3 exterior_target_pos =
                    intersection + this->plane_normal.direction * 0.00001f;
                bundle.scene->set_voxel_filled(this->depth, exterior_target_pos,
                                               bundle.current_color);
            }
        }
    }
}

auto VoxelBrush::handle_keyboard_event(const SDL_KeyboardEvent &event,
                                       const EventBundle &bundle) -> void {}
