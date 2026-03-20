#pragma once

#include <glm/glm.hpp>

namespace vxng::geometry {

typedef struct Ray {
    glm::vec3 origin;
    glm::vec3 direction;
} Ray;

typedef struct AABB {
    glm::vec3 min;
    glm::vec3 max;
} AABB;

/**
 * Checks if a ray intersects an axis-aligned box (AABB).
 */
auto ray_aabb_intersect(const Ray &ray, const AABB &aabb, float *t) -> bool;

/**
 * Computes the normal of the intersection point with an octant (AABB).
 */
auto compute_aabb_normal(const AABB &aabb, glm::vec3 intersection) -> glm::vec3;

} // namespace vxng::geometry
