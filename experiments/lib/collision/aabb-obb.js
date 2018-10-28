const { vec2 } = require('gl-matrix')

module.exports = collide

const axes = [
  [1, 0],
  [0, 1],
  // obb axes
  [0, 0],
  [0, 0],
  [0, 0],
  [0, 0]
]

const aabbCorners = [
  [0, 0],
  [0, 0],
  [0, 0],
  [0, 0]
]

const resultIntervals = [[0, 0], [0, 0]]

// Where aabb is specified by lower and upper extents
//   [[1, 1], [2, 3]]
// and obb is a set of vertices
//   [[0, 0], [1, 1], [0, 1], [-1, 0.5]]
function collide (aabb, obb) {
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

function dot (a, b) {
  return a[0] * b[0] + a[1] * b[1]
}

if (!module.parent) {
  const test = require('tape')

  test('basic aabb-aabb (collision)', (t) => {
    const r = collide(
      [[0, 0], [1, 1]],
      [
        [0.5, 0.5],
        [1.5, 0.5],
        [1.5, 1.5],
        [0.5, 1.5]
      ]
    )
    t.equal(r, true)
    t.end()
  })

  test('basic aabb-aabb (no collision)', (t) => {
    const r = collide(
      [[0, 0], [1, 1]],
      [
        [2, 2],
        [3, 2],
        [3, 3],
        [2, 3]
      ]
    )

    t.equal(r, false)

    t.end()
  })
}
