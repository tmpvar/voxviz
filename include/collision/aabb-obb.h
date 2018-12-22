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

/*
function collide(aabb, obb) {
  // bottom left
  aabbCorners[0][0] = aabb[0][0]
    aabbCorners[0][1] = aabb[0][1]
    // bottom right
    aabbCorners[1][0] = aabb[1][0]
    aabbCorners[1][1] = aabb[0][1]
    // top right
    aabbCorners[2][0] = aabb[1][0]
    aabbCorners[2][1] = aabb[1][1]
    // top left
    aabbCorners[3][0] = aabb[0][0]
    aabbCorners[3][1] = aabb[1][1]

    axes[2][0] = obb[1][0] - obb[0][0]
    axes[2][1] = obb[1][1] - obb[0][1]
    axes[3][0] = obb[3][0] - obb[0][0]
    axes[3][1] = obb[3][1] - obb[0][1]

    for (var axisIdx = 0; axisIdx < axes.length; axisIdx++) {
      const axis = vec2.normalize(vec2.create(), axes[axisIdx])
        resultIntervals[0][0] = Number.MAX_VALUE
        resultIntervals[0][1] = -Number.MAX_VALUE
        resultIntervals[1][0] = Number.MAX_VALUE
        resultIntervals[1][1] = -Number.MAX_VALUE
        for (var cornerIdx = 0; cornerIdx < 4; cornerIdx++) {
          var aabbProjection = dot(axis, aabbCorners[cornerIdx])
            var obbProjection = dot(axis, obb[cornerIdx])

            resultIntervals[0][0] = Math.min(resultIntervals[0][0], aabbProjection)
            resultIntervals[0][1] = Math.max(resultIntervals[0][1], aabbProjection)

            resultIntervals[1][0] = Math.min(resultIntervals[1][0], obbProjection)
            resultIntervals[1][1] = Math.max(resultIntervals[1][1], obbProjection)
        }

      if (
        resultIntervals[1][0] > resultIntervals[0][1] ||
        resultIntervals[1][1] < resultIntervals[0][0]
        ) {
        return false
      }
    }
  // TODO: compute and return the depth of intersection
  // console.log(
    // resultIntervals[0][1] > resultIntervals[1][1]
    // ? -(resultIntervals[1][1] - resultIntervals[0][0])
    // : resultIntervals[0][1] - resultIntervals[1][0]
  // )
  return true
}
*/