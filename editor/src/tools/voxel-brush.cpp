#include "voxel-brush.h"

#include "cursors.h"

VoxelBrush::VoxelBrush() {}

VoxelBrush::~VoxelBrush() {}

auto VoxelBrush::get_tool_name() -> const char * {
    return "Simple Voxel Brush";
}

// no ui for this yet
auto VoxelBrush::render_ui() -> void {}

auto VoxelBrush::handle_mouse_button_event(const MouseButtonEventBundle &bundle)
    -> void {
    // we don't care about mouse up
    if (!bundle.event->down)
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
    switch (bundle.event->button) {
    case SDL_BUTTON_LEFT: {
        // go outside of voxel boundary
        glm::vec3 exterior_target_pos =
            target_pos + raycast_result.normal * 0.00001f;

        bundle.scene->set_voxel_filled(9, exterior_target_pos,
                                       glm::u8vec4(255, 0, 0, 255));
        break;
    }
    case SDL_BUTTON_RIGHT: {
        // go inside voxel boundary
        glm::vec3 interior_target_pos =
            target_pos - raycast_result.normal * 0.00001f;
        bundle.scene->set_voxel_empty(9, interior_target_pos);
        break;
    }
    }
}

auto VoxelBrush::handle_mouse_motion_event(const MouseMotionEventBundle &bundle)
    -> void {
    auto mouse_ray = bundle.camera->screen_to_ray(bundle.mouse_ndc_coords);
    auto raycast_result = bundle.scene->raycast(mouse_ray);

    // set pointer to "cursor" if raycast hit something
    if (raycast_result.hit) {
        bundle.cursors->set_cursor(Cursors::Variant::POINTER);
    } else {
        bundle.cursors->set_cursor(Cursors::Variant::DEFAULT);
    }
}

auto VoxelBrush::handle_keyboard_event(const KeyboardEventBundle &bundle)
    -> void {}
