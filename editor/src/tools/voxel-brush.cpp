#include "voxel-brush.h"
#include "tools/draggable-tool.h"

#include <imgui.h>
#include <vxng/geometry.h>

VoxelBrush::VoxelBrush()
    : DraggableTool(5.f), current_mode(Mode::AXIS_ALIGNED), size(1), depth(9),
      plane_normal(), brush_kernel(size) {}

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
        this->brush_kernel.set_size(this->size);

    ImGui::SliderInt("Depth", &this->depth, 0, 9);
    this->render_flow_density_ui();
}

auto VoxelBrush::handle_mouse_button_event(const SDL_MouseButtonEvent &event,
                                           const EventBundle &bundle) -> void {
    DraggableTool::handle_mouse_button_event(event, bundle);

    // we don't care about mouse up
    if (!event.down)
        return;

    // only care about left and right click
    if (event.button != SDL_BUTTON_LEFT && event.button != SDL_BUTTON_RIGHT)
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

        stamp_brush(StampMode::PLACE, exterior_target_pos, bundle);
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

        stamp_brush(StampMode::DELETE, interior_target_pos, bundle);
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
    DraggableTool::handle_mouse_motion_event(event, bundle);

    auto mouse_ray = bundle.camera->screen_to_ray(bundle.mouse_ndc_coords);
    auto raycast_result = bundle.scene->raycast(mouse_ray);

    // set pointer to "cursor" if raycast hit something
    if (raycast_result.hit) {
        bundle.cursors->set_cursor(Cursors::Variant::POINTER);
    } else {
        bundle.cursors->set_cursor(Cursors::Variant::DEFAULT);
    }
}

auto VoxelBrush::handle_keyboard_event(const SDL_KeyboardEvent &event,
                                       const EventBundle &bundle) -> void {}

auto VoxelBrush::handle_drag_step(int step_idx, int total_steps,
                                  glm::vec2 step_mouse_ndc_coords,
                                  const EventBundle &bundle) -> void {

    bool is_lmb_dragging = this->is_mousebutton_dragging(SDL_BUTTON_LEFT);
    bool is_rmb_dragging = this->is_mousebutton_dragging(SDL_BUTTON_RIGHT);

    if (is_lmb_dragging || is_rmb_dragging) {
        bool skip_update_buffers = step_idx < total_steps - 1;

        auto mouse_ray = bundle.camera->screen_to_ray(step_mouse_ndc_coords);

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

                StampMode stamp_mode =
                    (is_lmb_dragging) ? StampMode::PLACE : StampMode::DELETE;
                // add/remove voxels
                stamp_brush(stamp_mode, intersection, bundle,
                            skip_update_buffers);
            }
        }
    }
}

auto VoxelBrush::stamp_brush(StampMode mode, glm::vec3 position,
                             const EventBundle &bundle,
                             bool skip_update_buffers) -> void {
    float voxel_size =
        bundle.scene->get_chunk_scale() / (float)(1u << this->depth);

    for (auto &ioffset : this->brush_kernel.get_kernel()) {
        if (mode == StampMode::PLACE) {
            bundle.scene->set_voxel_filled(
                this->depth, position + glm::vec3(ioffset) * voxel_size,
                bundle.current_color, true);
        } else {
            bundle.scene->set_voxel_empty(
                this->depth, position + glm::vec3(ioffset) * voxel_size, true);
        }
    }

    // set base node
    if (mode == StampMode::PLACE) {
        bundle.scene->set_voxel_filled(
            this->depth, position, bundle.current_color, skip_update_buffers);
    } else {
        bundle.scene->set_voxel_empty(this->depth, position,
                                      skip_update_buffers);
    }
}
