const { vec2 } = require('gl-matrix')

module.exports = collide

const axes = [
  [0, 0],
  [0, 0],
  [0, 0],
  [0, 0],
]

const resultIntervals = [[0, 0], [0, 0]]

// and obb2 is a set of vertices
//   [[0, 0], [1, 1], [0, 1], [-1, 0.5]]
function collide (obb1, obb2) {
  axes[0][0] = obb1[1][0] - obb1[0][0]
  axes[0][1] = obb1[1][1] - obb1[0][1]
  axes[1][0] = obb1[3][0] - obb1[0][0]
  axes[1][1] = obb1[3][1] - obb1[0][1]

  axes[2][0] = obb2[1][0] - obb2[0][0]
  axes[2][1] = obb2[1][1] - obb2[0][1]
  axes[3][0] = obb2[3][0] - obb2[0][0]
  axes[3][1] = obb2[3][1] - obb2[0][1]

  for (var axisIdx = 0; axisIdx < axes.length; axisIdx++) {
    const axis = vec2.normalize(vec2.create(), axes[axisIdx])
    resultIntervals[0][0] = Number.MAX_VALUE
    resultIntervals[0][1] = -Number.MAX_VALUE
    resultIntervals[1][0] = Number.MAX_VALUE
    resultIntervals[1][1] = -Number.MAX_VALUE
    for (var cornerIdx = 0; cornerIdx < 4; cornerIdx++) {
      var obb1Projection = dot(axis, obb1[cornerIdx])
      var obb2Projection = dot(axis, obb2[cornerIdx])

      resultIntervals[0][0] = Math.min(resultIntervals[0][0], obb1Projection)
      resultIntervals[0][1] = Math.max(resultIntervals[0][1], obb1Projection)

      resultIntervals[1][0] = Math.min(resultIntervals[1][0], obb2Projection)
      resultIntervals[1][1] = Math.max(resultIntervals[1][1], obb2Projection)
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
  //   resultIntervals[0][1] > resultIntervals[1][1]
  //   ? -(resultIntervals[1][1] - resultIntervals[0][0])
  //   : resultIntervals[0][1] - resultIntervals[1][0]
  // )
  return true
}

function dot (a, b) {
  return a[0] * b[0] + a[1] * b[1]
}
