const center = require('ctx-translate-center')
const {vec2} = require('gl-matrix')
const Volume = require('./lib/volume')
const { BRICK_DIAMETER } = require('./lib/brick')
const createCascade = require('./lib/voxel-cascade')

const cascadeDiameter = 16
const cascadeCount = 5
const cascades = []
for (var i = 0; i<cascadeCount; i++) {
  cascades.push(createCascade(i+1, cascadeDiameter, cascadeCount))
}

const stock = new Volume([0, 0])
stock.addBrick([0, 0]).fill((x, y) => {
  const v = 1.0 + (x + y * BRICK_DIAMETER) / Math.pow(BRICK_DIAMETER, 2)
  return v
}).empty = false

const volumes = [new Volume([10, 0]), new Volume([0, 0]), stock]

volumes[0].addBrick([0, 0]).fill((x, y) => {
  return 1;
}).empty = false

volumes[0].scale = [10, 10]

// for (var x = -50; x<=50; x+=20) {
//   for (var y = -50; y<=50; y+=20) {
//     volumes[1].addBrick([x, y]).fill(_ => 1).empty=false
//   }
// }

const ctx = require('fc')(render, 1)
const camera = require('ctx-camera')(ctx, window, {})

function render() {
  stock.rotation += 0.01
  //stock.pos[0] = -25
  //stock.pos[1] = 25

  volumes[0].pos = [Math.sin(Date.now() / 1000) * 200, Math.cos(Date.now() / 1000) * 25]
  stock.scale = [Math.abs(Math.sin(Date.now() / 1000) + 2) * 50, 1.0]
  ctx.clear()
  camera.begin()
    center(ctx)
    ctx.scale(10, -10)
    ctx.lineCap = "round"
    ctx.lineWidth= .2

    // Render cascades from course to fine
    for (var i=cascadeCount-1; i>=0; i--) {
      // TODO: reset the cascades
      cascades[i].center = [
        Math.sin(Date.now() / 10000) * 100,
        Math.cos(Date.now() / 10000) * 100
      ]
      cascades[i].reset()
      cascades[i].render(ctx)

      // TODO: consider using the previous, coarser, cascade as a source of data
      volumes.forEach((volume) => {
        cascades[i].addVolume(ctx, volume.modelMatrix, volume)
      })
    }

    volumes.forEach((volume) => volume.render(ctx))
  camera.end()
}

