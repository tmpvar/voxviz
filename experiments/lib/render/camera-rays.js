const {vec2} = require('gl-matrix')
const raySlab = require('ray-aabb-slab')

module.exports = drawCameraRays

function drawCameraRays(fov, eye, target, march) {
  var dx = target[0] - eye[0]
  var dy = target[1] - eye[1]
  var dir = [dx, dy]
  var nDir = vec2.normalize(vec2.create(), dir)

  var numRays = 16;
  // var fov = Math.PI/16
  var start = -fov/2
  var step = fov/numRays
  var rayDir = vec2.fromValues(1, 0)
  var zero = [0, 0]
  var invRayDir = vec2.create()
  for (var i=0; i<=numRays; i++) {
    vec2.rotate(rayDir, dir, zero, start + i * step)
    vec2.normalize(rayDir, rayDir)
    vec2.set(invRayDir, 1/rayDir[0], 1/rayDir[1])

    var ray = {
      dir: rayDir,
      nDir: nDir,
      invDir: invRayDir,
      origin: eye
    }

    if (march) {
      march(ray, i, numRays)
    }
  }
}
