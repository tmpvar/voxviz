#include "../include/core.h"

float mincomp(vec3 v) {
  return min(v.x, min(v.y, v.z));
}
