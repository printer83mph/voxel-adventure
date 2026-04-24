#include "voxel-brush.h"

#include <imgui.h>
#include <vxng/geometry.h>

VoxelBrush::VoxelBrush()
    : current_mode(Mode::AXIS_ALIGNED), size(1), depth(9), flow_density(5.f),
      plane_normal(), last_mouse_ndc_coords(), abs_brush_kernel() {}

VoxelBrush::~VoxelBrush() {}

auto VoxelBrush::get_tool_name() -> const char * { return "Voxel Brush"; }

// no ui for this yet
auto VoxelBrush::render_ui() -> void {
    if (ImGui::RadioButton("Axis aligned",
                           this->current_mode == Mode::AXIS_ALIGNED))
        this->current_mode = Mode::AXIS_ALIGNED;
    if (ImGui::RadioButton("Camera plane",
                           this->current_mode == Mode::CAMERA_PLANE))
        this->current_mode = Mode::CAMERA_PLANE;

    if (ImGui::SliderInt("Size", &this->size, 1, 5))
        compute_brush_kernel();

    ImGui::SliderInt("Depth", &this->depth, 0, 9);
    ImGui::SliderFloat("Flow Density", &this->flow_density, 0.f, 20.f);
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

    glm::vec3 plane_normal = this->current_mode == Mode::AXIS_ALIGNED
                                 ? raycast_result.normal
                                 : bundle.camera->get_forward();
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
            .direction = plane_normal,
        };
        break;
    }
    case SDL_BUTTON_RIGHT: {
        // go inside voxel boundary
        glm::vec3 interior_target_pos =
            target_pos - raycast_result.normal * 0.00001f;
        bundle.scene->set_voxel_empty(this->depth, interior_target_pos);
        this->plane_normal = vxng::geometry::Ray{
            .origin = interior_target_pos,
            .direction = plane_normal,
        };
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

    // do dragging logic
    if (event.state & (SDL_BUTTON_LMASK | SDL_BUTTON_RMASK)) {
        float mouse_move_distance =
            ((bundle.mouse_ndc_coords - this->last_mouse_ndc_coords) *
             glm::vec2(bundle.camera->get_aspect_ratio(), 1.f))
                .length();

        int flow_step_count =
            glm::ceil(mouse_move_distance * this->flow_density);

        for (int step = 1; step <= flow_step_count; ++step) {
            glm::vec2 lerped_mouse_ndc_coords =
                glm::mix(this->last_mouse_ndc_coords, bundle.mouse_ndc_coords,
                         (float)step / (float)flow_step_count);
            bool skip_update_buffers = step < flow_step_count;

            auto mouse_ray =
                bundle.camera->screen_to_ray(lerped_mouse_ndc_coords);

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
                    if (event.state & SDL_BUTTON_LEFT) {
                        // add voxels
                        bundle.scene->set_voxel_filled(
                            this->depth, intersection, bundle.current_color,
                            skip_update_buffers);
                    } else {
                        // remove voxels
                        bundle.scene->set_voxel_empty(this->depth,
                                                      intersection);
                    }
                }
            }
        }
    }

    this->last_mouse_ndc_coords = bundle.mouse_ndc_coords;
}

auto VoxelBrush::handle_keyboard_event(const SDL_KeyboardEvent &event,
                                       const EventBundle &bundle) -> void {}

auto VoxelBrush::compute_brush_kernel() -> void {
    this->abs_brush_kernel.clear();

    for (int x = 0; x < this->size; ++x) {
        for (int y = 0; y < this->size; ++y) {
            for (int z = 0; z < this->size; ++z) {
                float magnitude = glm::sqrt(x * x + y * y + z * z);
                this->abs_brush_kernel[glm::uvec3(x, y, z)] =
                    magnitude < this->size - 0.000001f;
            }
        }
    }
}
