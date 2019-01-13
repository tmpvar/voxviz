const tx = require('../tx')
const {mat3} = require('gl-matrix')
const cameraLookAt = require('../lookat')

module.exports = drawCamera
var a = mat3.create()
function drawCamera(ctx, eye, origin, scale) {
  cameraLookAt(a, eye, origin)
  scale = scale || 1
  const lineTo = ctx.lineTo.bind(ctx)
  const moveTo = ctx.moveTo.bind(ctx)

  ctx.save()
  ctx.beginPath()
    tx(a, - 5 * scale, - 5 * scale, moveTo)
    tx(a, + 5 * scale, - 5 * scale, lineTo)
    tx(a, + 5 * scale, + 5 * scale, lineTo)
    tx(a, - 5 * scale, + 5 * scale, lineTo)
    tx(a, - 5 * scale, - 5 * scale, lineTo)


    tx(a, 0,  0, moveTo)
    tx(a, 10 * scale, - 5 * scale, lineTo)
    tx(a, 10 * scale, + 5 * scale, lineTo)
    tx(a, 0, 0, moveTo)
    ctx.fill()
  ctx.restore()
}
