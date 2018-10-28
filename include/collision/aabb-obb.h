#pragma once



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