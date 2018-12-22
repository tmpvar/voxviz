#include "aabb.h"
#include <iostream>

using namespace std;

void aabb_print(aabb_t *v) {
  cout << "lower("
    << v->lower.x << ", "
    << v->lower.y << ", "
    << v->lower.z << ") upper("
    << v->upper.x << ", "
    << v->upper.y << ", "
    << v->upper.z << ")"
    << endl << endl;
}

aabb_t *aabb_create() {
  aabb_t *out = (aabb_t *)malloc(sizeof(aabb_t));
  memset(out, 0, sizeof(aabb_t));
  return out;
}

void aabb_free(aabb_t *a) {
  if (a == nullptr) {
    return;
  }

  free(a);
  a = nullptr;
}

void aabb_grow(aabb_t *dst, const aabb_t *src) {
  dst->lower = glm::min(src->lower, dst->lower);
  dst->upper = glm::max(src->upper, dst->upper);
}

bool aabb_isect(const aabb_t *a, const aabb_t *b, aabb_t *out) {
  if (!aabb_overlaps(a, b)) {
    return false;
  }

  /*out->upper = min(a->upper, b->upper);
  out->lower = max(a->lower, b->lower);
  */
  out->upper.x = glm::min(a->upper.x, b->upper.x);
  out->upper.y = glm::min(a->upper.y, b->upper.y);
  out->upper.z = glm::min(a->upper.z, b->upper.z);

  out->lower.x = glm::max(a->lower.x, b->lower.x);
  out->lower.y = glm::max(a->lower.y, b->lower.y);
  out->lower.z = glm::max(a->lower.z, b->lower.z);
  
  return true;
}

bool aabb_contains(const aabb_t *box, const glm::vec3 v) {
  return glm::all(glm::greaterThanEqual(v, box->lower)) &&
    glm::all(glm::lessThanEqual(v, box->upper));
}

bool aabb_overlaps(const aabb_t *a, const aabb_t *b) {
  return aabb_overlaps_component(
    a->lower,
    a->upper,
    b->lower,
    b->upper
  );
}

bool aabb_overlaps_component(glm::vec3 a_lower, glm::vec3 a_upper, glm::vec3 b_lower, glm::vec3 b_upper) {
  return (
    (a_lower.x <= b_upper.x && a_upper.x >= b_lower.x) &&
    (a_lower.y <= b_upper.y && a_upper.y >= b_lower.y) &&
    (a_lower.z <= b_upper.z && a_upper.z >= b_lower.z)
    );
}