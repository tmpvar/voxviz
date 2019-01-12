const {vec2, mat3} = require('gl-matrix')

module.exports = cameraLookAt

const v2tmpCam = vec2.create()

function cameraLookAt(mat, eye, target) {
  vec2.subtract(v2tmpCam, target, eye)
  mat3.identity(mat)

  const rot = Math.atan2(v2tmpCam[1], v2tmpCam[0])
  mat3.translate(mat, mat, eye)
  mat3.rotate(mat, mat, rot)
  return mat
}
