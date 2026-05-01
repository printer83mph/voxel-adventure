#include "vxng/scene.h"

#include "chunk.h"

#include <ogt/ogt_vox.h>

#include <array>
#include <iostream>
#include <limits>
#include <memory>
#include <stdexcept>
#include <unordered_map>
#include <vector>

#define DEFAULT_CHUNK_SCALE 4.0f
#define DEFAULT_CHUNK_RESOLUTION 512

namespace vxng::scene {

Scene::Scene(int chunk_resolution, float chunk_scale)
    : chunk_resolution(chunk_resolution), chunk_scale(chunk_scale) {
    if (chunk_resolution <= 0 ||
        !((chunk_resolution & (chunk_resolution - 1)) == 0)) {
        throw std::invalid_argument("Chunk resolution must be a power of 2");
    }

    if (chunk_scale <= 0) {
        throw std::invalid_argument("Chunk scale must be greater than 0");
    }
}

Scene::Scene()
    : chunk_resolution(DEFAULT_CHUNK_RESOLUTION),
      chunk_scale(DEFAULT_CHUNK_SCALE) {}

Scene::~Scene() {}

auto Scene::init_webgpu(wgpu::Device device) -> void {
    this->wgpu.device = device;
    touch_chunk(glm::ivec3(0.f));
}

auto Scene::get_chunks() const
    -> const std::unordered_map<glm::ivec3, std::unique_ptr<Chunk>> & {
    return this->chunks;
}

auto Scene::fill_basic_plane(glm::u8vec4 color) -> void {
    // set 4 base plates filled
    float delta = this->chunk_scale / (float)this->chunk_resolution * 0.5f;

    this->set_voxel_filled(1, {delta, -delta, delta}, color);
    this->set_voxel_filled(1, {-delta, -delta, delta}, color);
    this->set_voxel_filled(1, {-delta, -delta, -delta}, color);
    this->set_voxel_filled(1, {delta, -delta, -delta}, color);
}

auto Scene::fill_center_cubes(int depth, glm::u8vec4 color) -> void {
    // set 8 center cubes filled
    float delta = this->chunk_scale / (float)this->chunk_resolution * 0.5f;

    for (int x = -1; x <= 1; x += 2) {
        for (int y = -1; y <= 1; y += 2) {
            for (int z = -1; z <= 1; z += 2) {
                this->set_voxel_filled(depth, glm::vec3(x, y, z) * delta,
                                       color);
            }
        }
    }
}

auto Scene::load_vox_file(const std::vector<uint8_t> &buffer) -> void {
    const ogt_vox_scene *scene = ogt_vox_read_scene(&buffer[0], buffer.size());

    // do stuff with scene...
    for (int i = 0; i < scene->num_models; ++i) {
        const ogt_vox_model *model = scene->models[i];

        std::cout << "model found: " << model->size_x << "x" << model->size_y
                  << "x" << model->size_z << std::endl;

        int voxel_count = model->size_x * model->size_y * model->size_z;

        std::array<glm::u8vec4, 256> palette = {};
        for (int i = 0; i < 256; ++i) {
            const ogt_vox_rgba &color = scene->palette.color[i];
            palette[i] = glm::u8vec4(color.r, color.g, color.b, 255);
        }

        // convert coordinate systems (swap Y and Z)
        // MagicaVoxel: X, Y (up), Z
        // Our scene:   X, Z (up), Y
        std::vector<uint8_t> transformed_voxel_data(voxel_count, 0);

        for (int x = 0; x < model->size_x; ++x) {
            for (int y = 0; y < model->size_y; ++y) {
                for (int z = 0; z < model->size_z; ++z) {
                    // Original index (MagicaVoxel coordinates)
                    int src_idx = x + (y * model->size_x) +
                                  (z * model->size_x * model->size_y);

                    // New index (swapped Y and Z)
                    int dst_idx = x + (z * model->size_x) +
                                  (y * model->size_x * model->size_z);

                    transformed_voxel_data[dst_idx] =
                        model->voxel_data[src_idx];
                }
            }
        }

        this->chunks[{0, 0, 0}]->set_voxel_grid_data(
            transformed_voxel_data.data(),
            {model->size_x, model->size_y, model->size_z}, palette,
            {256, 256, 256});
    }

    ogt_vox_destroy_scene(scene);
}

auto Scene::write_vox_buffer(uint32_t *buffer_size) const -> uint8_t * {
    // scene to output
    ogt_vox_scene scene = {};

    // just 1 model, nothing else
    scene.num_models = 1;
    scene.num_cameras = 0;
    scene.num_color_names = 0;
    scene.num_groups = 0;
    scene.num_instances = 0;
    scene.num_layers = 0;
    scene.anim_range_start = 0;
    scene.anim_range_end = 0;

    // collect palette into array (first element is blank)
    std::vector<glm::u8vec4> palette = {};
    palette.reserve(256);
    palette.push_back({0, 0, 0, 0});
    std::unordered_map<glm::u8vec4, int> color_to_palette_idx = {};

    for (auto &chunk_pair : this->chunks) {
        if (chunk_pair.second->is_empty())
            continue; // skip chunk if empty

        // collect colors from chunk and add to vector
        auto chunk_colors = chunk_pair.second->collect_colors();
        for (auto color : chunk_colors) {
            auto existing_color_idx = color_to_palette_idx.find(color);
            if (existing_color_idx != color_to_palette_idx.end())
                continue; // skip if we already counted this color

            color_to_palette_idx[color] = palette.size();
            palette.push_back(color);
        }
    }

    if (palette.size() > 255) {
        throw std::runtime_error(
            "More than 255 colors were found in the scene!");
    }

    scene.palette = {};
    for (int i = 0; i < 256; ++i) {
        auto color = palette[i];
        scene.palette.color[i] = ogt_vox_rgba{
            .r = color.r, .g = color.g, .b = color.b, .a = color.a};
    }

    // figure out full scene bounds
    glm::ivec3 min_chunk = glm::ivec3(std::numeric_limits<int>::max());
    glm::ivec3 max_chunk = glm::ivec3(std::numeric_limits<int>::min());
    for (auto &chunk_pair : this->chunks) {
        min_chunk = glm::min(min_chunk, chunk_pair.first);
        max_chunk = glm::max(max_chunk, chunk_pair.first);
    }
    glm::ivec3 dimensions_in_chunks = max_chunk - min_chunk;
    glm::ivec3 dimensions_in_voxels =
        dimensions_in_chunks * this->chunk_resolution;

    // iterate through chunks and insert data into array
    std::vector<uint8_t> voxel_data;
    voxel_data.reserve(dimensions_in_voxels.x * dimensions_in_voxels.y *
                       dimensions_in_voxels.z);

    float local_voxel_size = 1.f / this->chunk_resolution;
    glm::vec3 voxel_start_sample_pos =
        glm::vec3(-0.5f) + glm::vec3(local_voxel_size * 0.5f);
    for (auto &chunk_pair : this->chunks) {
        const Chunk *chunk = chunk_pair.second.get();

        auto chunk_voxel_offset =
            (chunk_pair.first - min_chunk) * this->chunk_resolution;

        for (int x = 0; x < this->chunk_resolution; ++x) {
            for (int y = 0; y < this->chunk_resolution; ++y) {
                for (int z = 0; z < this->chunk_resolution; ++z) {
                    glm::vec3 local_sample_position =
                        voxel_start_sample_pos + glm::vec3() * local_voxel_size;
                    auto sample = chunk->sample_position(local_sample_position);

                    auto global_voxel_coord =
                        chunk_voxel_offset + glm::ivec3(x, y, z);

                    int destination_index =
                        global_voxel_coord.x +
                        (global_voxel_coord.z * dimensions_in_voxels.x) +
                        (global_voxel_coord.y * dimensions_in_voxels.x *
                         dimensions_in_voxels.z);

                    if (sample.has_value()) {
                        voxel_data[destination_index] =
                            color_to_palette_idx[sample.value()];
                    } else {
                        voxel_data[destination_index] = 0u;
                    }
                }
            }
        }

        vxng::geometry::AABB bounds = chunk->get_bounds();
    }

    ogt_vox_model model;

    const ogt_vox_model *model_ptr = &model;
    scene.models = &model_ptr;

    return ogt_vox_write_scene(&scene, buffer_size);
}

auto Scene::destroy_vox_buffer(uint8_t *buffer) const -> void {
    ogt_vox_free(buffer);
}

auto Scene::sample_position(glm::vec3 position) const
    -> std::optional<glm::u8vec4> {
    auto chunked_info = get_chunked_location_info(position);
    auto chunk = this->chunks.find(chunked_info.chunk_coord);

    if (chunk == this->chunks.end()) {
        return {}; // no chunk initialized here, return nothing
    }

    // chunk exists - sample it!
    return chunk->second->sample_position(chunked_info.local_position);
}

auto Scene::raycast(const geometry::Ray &ray) const -> geometry::RaycastResult {
    // scan through chunks for any intersections
    geometry::RaycastResult closest_hit;
    closest_hit.hit = false;
    closest_hit.t = std::numeric_limits<float>::max();

    for (const auto &chunk_pair : this->chunks) {
        Chunk *chunk = chunk_pair.second.get();
        geometry::AABB chunk_bounds = chunk->get_bounds();
        if (!geometry::ray_aabb_intersect(ray, chunk_bounds).hit)
            continue;

        // this ray intersects this chunk, let's continue with raycast
        geometry::RaycastResult chunk_result = chunk->raycast(ray);
        if (chunk_result.hit && chunk_result.t < closest_hit.t)
            closest_hit = chunk_result;
    }

    return closest_hit;
}

auto Scene::set_voxel_filled(int depth, glm::vec3 position, glm::u8vec4 color,
                             bool skip_update_buffers) -> void {
    auto chunked_location = get_chunked_location_info(position);
    Chunk *target_chunk = touch_chunk(chunked_location.chunk_coord);

    target_chunk->set_voxel_filled(depth, chunked_location.local_position,
                                   color, skip_update_buffers);
}

auto Scene::set_voxel_empty(int depth, glm::vec3 position,
                            bool skip_update_buffers) -> void {
    auto chunked_location = get_chunked_location_info(position);
    Chunk *target_chunk = touch_chunk(chunked_location.chunk_coord);

    target_chunk->set_voxel_empty(depth, chunked_location.local_position,
                                  skip_update_buffers);
}

auto Scene::get_chunk_scale() const -> float { return this->chunk_scale; }

auto Scene::get_chunk_resolution() const -> int {
    return this->chunk_resolution;
}

auto Scene::set_chunk_scale(float new_scale) -> void {
    this->chunk_scale = new_scale;

    for (const auto &chunk_pair : this->chunks) {
        glm::ivec3 chunk_ipos = chunk_pair.first;
        glm::vec3 chunk_pos = glm::vec3(chunk_ipos) * new_scale;

        chunk_pair.second->reposition(chunk_pos, new_scale);
    }
}

auto Scene::get_chunked_location_info(glm::vec3 position) const
    -> ChunkedLocationInfo {
    glm::ivec3 chunk_coord = glm::floor(
        (position + glm::vec3(this->chunk_scale * 0.5f)) / this->chunk_scale);
    glm::vec3 chunk_origin = glm::vec3(chunk_coord) * this->chunk_scale;
    glm::vec3 local_position = (position - chunk_origin) / this->chunk_scale;

    return ChunkedLocationInfo{
        .chunk_coord = chunk_coord,
        .local_position = local_position,
    };
}

auto Scene::touch_chunk(glm::ivec3 chunk_coord) -> Chunk * {
    // if chunk already exists just grab it
    if (this->chunks.find(chunk_coord) != this->chunks.end()) {
        return this->chunks[chunk_coord].get();
    }

    // otherwise, we gotta create the chunk
    glm::vec3 chunk_origin = glm::vec3(chunk_coord) * this->chunk_scale;
    this->chunks[chunk_coord] = std::make_unique<Chunk>(
        chunk_origin, this->chunk_scale, this->chunk_resolution);
    auto &new_chunk = this->chunks[chunk_coord];

    // and init webgpu
    new_chunk->init_webgpu(this->wgpu.device);

    return new_chunk.get();
}

} // namespace vxng::scene
