#ifndef __AABB_H__
#define __AABB_H__

#include <glm/glm.hpp>

typedef struct {
  glm::vec3 lower;
  glm::vec3 upper;
} aabb_t;

void aabb_print(aabb_t v);

#endif
