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

static bool collision_aabb_obb(const glm::vec3 aabb[8], const glm::vec3 obb[8]) {
  glm::vec3 axes[6] = {
    glm::vec3(1.0, 0.0, 0.0),
    glm::vec3(0.0, 1.0, 0.0),
    glm::vec3(0.0, 0.0, 1.0),
    glm::normalize(obb[1] - obb[0]),
    glm::normalize(obb[2] - obb[0]),
    glm::normalize(obb[4] - obb[0])
  };
  double inf = std::numeric_limits<double>::infinity();
  glm::vec2 resultIntervals[2];

  for (int axisIdx = 0; axisIdx < 6; axisIdx++) {
    resultIntervals[0].x = inf;
    resultIntervals[0].y = -inf;
    resultIntervals[1].x = inf;
    resultIntervals[1].y = -inf;
    glm::vec3 axis = axes[axisIdx];

    for (int cornerIdx = 0; cornerIdx < 8; cornerIdx++) {
      auto aabbProjection = glm::dot(axis, aabb[cornerIdx]);
      auto obbProjection = glm::dot(axis, obb[cornerIdx]);

      resultIntervals[0].x = min(resultIntervals[0].x, aabbProjection);
      resultIntervals[0].y = max(resultIntervals[0].y, aabbProjection);

      resultIntervals[1].x = min(resultIntervals[1].x, obbProjection);
      resultIntervals[1].y = max(resultIntervals[1].y, obbProjection);
    }

    if (
      resultIntervals[1].x > resultIntervals[0].y ||
      resultIntervals[1].y < resultIntervals[0].x
      ) {
      return false;
    }
  }
  return true;
}


bool collision_aabb_obb_old(glm::vec3 aabb_lower, glm::vec3 aabb_upper, glm::vec3 obb[8]) {
  
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