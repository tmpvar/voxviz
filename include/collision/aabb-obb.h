#pragma once

#include "glm/glm.hpp"

// this assumes the order of the obb is as follows
/*
     6 o---------o 7
      /.        /|
   2 / .       / |
    o---------o 3|
    |  .      |  |
    |4 o . . .|. o 5
    | .       | /
    |.        |/
    o---------o
    0         1
*/
bool collision_aabb_obb(glm::vec3 aabb_lower, glm::vec3 aabb_upper, glm::vec3 obb[8]) {
  
  glm::vec3 axes[6] = {
    // aabb axes
    glm::vec3(0, 0, 1),
    glm::vec3(0, 1, 0),
    glm::vec3(1, 0, 0),
    
    // obb axes
    obb[1] - obb[0],
    obb[3] - obb[0],
    obb[4] - obb[0]
  };

  for (uint8_t axisIdx = 0; axisIdx<6; axisIdx++) {
    glm::vec3 axis = axes[axisIdx];

    glm::vec2 aabbInterval = glm::vec2(FLT_MAX, -FLT_MAX);
    glm::vec2 obbInterval = glm::vec2(FLT_MAX, -FLT_MAX);

    for (uint8_t i = 0; i < 8; i++) {
      glm::vec3 aabbCorner = glm::vec3(
        (i & 4) > 0 ? aabb_upper.x : aabb_lower.x,
        (i & 2) > 0 ? aabb_upper.y : aabb_lower.y,
        (i & 1) > 0 ? aabb_upper.z : aabb_lower.z
      );

      float aabbProjection = glm::dot(axis, aabbCorner);
      float obbProjection = glm::dot(axis, obb[i]);
      aabbInterval.x = min(aabbInterval.x, aabbProjection);
      aabbInterval.y = max(aabbInterval.y, aabbProjection);

      obbInterval.x = min(obbInterval.x, obbProjection);
      obbInterval.y = max(obbInterval.y, obbProjection);
    }

    if (obbInterval.x > aabbInterval.y || obbInterval.y < aabbInterval.x) {
      return false;
    }
  }  

  return true;
}