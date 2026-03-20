#include "geometry.h"

namespace vxng::geometry {

// implementation based on libs/vxng/src/wgsl/chunk.wgsl.cpp
auto ray_aabb_intersect(const Ray &ray, const AABB &aabb, float *t) -> bool {
    const float invDirX = 1.0f / ray.direction.x;
    const float invDirY = 1.0f / ray.direction.y;
    const float invDirZ = 1.0f / ray.direction.z;

    float tNear = 0.0f;
    float tFar = std::numeric_limits<float>::max();

    float t0X = (aabb.min.x - ray.origin.x) * invDirX;
    float t1X = (aabb.max.x - ray.origin.x) * invDirX;

    float t0Y = (aabb.min.y - ray.origin.y) * invDirY;
    float t1Y = (aabb.max.y - ray.origin.y) * invDirY;

    float t0Z = (aabb.min.z - ray.origin.z) * invDirZ;
    float t1Z = (aabb.max.z - ray.origin.z) * invDirZ;

    tNear = std::max({t0X, t1X, t0Y, t1Y, t0Z, t1Z});

    tFar = std::min({t0X, t1X, t0Y, t1Y, t0Z, t1Z});

    if (tNear > tFar) {
        return false;
    }

    *t = tNear;

    return true;
}

auto compute_aabb_normal(const AABB &aabb, glm::vec3 intersection)
    -> glm::vec3 {
    const float epsilon = 0.0001f;

    if (std::abs(intersection.x - aabb.min.x) < epsilon) {
        return glm::vec3(-1.0f, 0.0f, 0.0f);
    } else if (std::abs(intersection.x - aabb.max.x) < epsilon) {
        return glm::vec3(1.0f, 0.0f, 0.0f);
    } else if (std::abs(intersection.y - aabb.min.y) < epsilon) {
        return glm::vec3(0.0f, -1.0f, 0.0f);
    } else if (std::abs(intersection.y - aabb.max.y) < epsilon) {
        return glm::vec3(0.0f, 1.0f, 0.0f);
    } else if (std::abs(intersection.z - aabb.min.z) < epsilon) {
        return glm::vec3(0.0f, 0.0f, -1.0f);
    } else if (std::abs(intersection.z - aabb.max.z) < epsilon) {
        return glm::vec3(0.0f, 0.0f, 1.0f);
    }

    // evil fallback just in case, shouldn't happen
    return glm::vec3(0.0f, 0.0f, 0.0f);
}

} // namespace vxng::geometry
