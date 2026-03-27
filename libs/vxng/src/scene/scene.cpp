#include "vxng/scene.h"

#include "chunk.h"

#include <ogt/ogt_vox.h>

#include <iostream>
#include <limits>
#include <memory>
#include <stdexcept>

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

auto Scene::load_vox_file(const std::vector<uint8_t> &buffer) -> void {
    const ogt_vox_scene *scene = ogt_vox_read_scene(&buffer[0], buffer.size());

    // do stuff with scene...
    for (int i = 0; i < scene->num_models; ++i) {
        const ogt_vox_model *model = scene->models[i];

        std::cout << "model found: " << model->size_x << "x" << model->size_y
                  << "x" << model->size_z << std::endl;
    }

    ogt_vox_destroy_scene(scene);
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

auto Scene::set_voxel_filled(int depth, glm::vec3 position, glm::u8vec4 color)
    -> void {
    auto chunked_location = get_chunked_location_info(position);
    Chunk *target_chunk = touch_chunk(chunked_location.chunk_coord);

    target_chunk->set_voxel_filled(depth, chunked_location.local_position,
                                   color);
}

auto Scene::set_voxel_empty(int depth, glm::vec3 position) -> void {
    auto chunked_location = get_chunked_location_info(position);
    Chunk *target_chunk = touch_chunk(chunked_location.chunk_coord);

    target_chunk->set_voxel_empty(depth, chunked_location.local_position);
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
