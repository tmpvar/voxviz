const tx = require('../tx')
const {mat3} = require('gl-matrix')
const cameraLookAt = require('../lookat')

module.exports = drawCamera
var a = mat3.create()
function drawCamera(ctx, eye, origin) {
  cameraLookAt(a, eye, origin)

  const lineTo = ctx.lineTo.bind(ctx)
  const moveTo = ctx.moveTo.bind(ctx)

  ctx.save()
  ctx.beginPath()
    tx(a, -5, -5, moveTo)
    tx(a, +5, -5, lineTo)
    tx(a, +5, +5, lineTo)
    tx(a, -5, +5, lineTo)
    tx(a, -5, -5, lineTo)
    ctx.stroke()
    ctx.fill()

  ctx.beginPath()
    tx(a, 0,  0, moveTo)
    tx(a, +10, -5, lineTo)
    tx(a, +10, +5, lineTo)
    tx(a, 0,  0, moveTo)
    ctx.stroke()
    ctx.fill()
  ctx.restore()
}
