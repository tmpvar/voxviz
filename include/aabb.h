#ifndef __AABB_H__
#define __AABB_H__

#include <glm/glm.hpp>

typedef struct {
  glm::vec3 lower;
  glm::vec3 upper;
} aabb_t;

void aabb_print(aabb_t *v);
aabb_t * aabb_create();
void aabb_free(aabb_t *a);

void aabb_grow(aabb_t * dst, const aabb_t * src);
bool aabb_isect(const aabb_t *a, const aabb_t *b, aabb_t *out);

bool aabb_contains(const aabb_t * box, const glm::vec3 v);

bool aabb_overlaps(const aabb_t * a, const aabb_t * b);
bool aabb_overlaps_component(glm::vec3 a_lower, glm::vec3 a_upper, glm::vec3 b_lower, glm::vec3 b_upper);
#endif
