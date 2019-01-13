
const center = require('ctx-translate-center')
const glmatrix = require('gl-matrix')
const vec2 = glmatrix.vec2
const mat3 = glmatrix.mat3
const segseg = require('segseg')
const raySlab = require('ray-aabb-slab')
const Volume = require('./lib/volume')
const { BRICK_DIAMETER } = require('./lib/brick')
const createCascade = require('./lib/voxel-cascade')
const cameraLookAt = require('./lib/lookat')
const drawCamera = require('./lib/render/camera')
const drawCameraRays = require('./lib/render/camera-rays')
const createKeyboard = require('./lib/keyboard')
const keyboard = createKeyboard()
const cascadeDiameter = 16
const cascadeCount = 5
const cascades = []
for (var i = 0; i<cascadeCount; i++) {
  cascades.push(createCascade(i+1, cascadeDiameter, cascadeCount))
}

const min = Math.min
const max = Math.max
const floor = Math.floor
const ceil = Math.ceil

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

const ctx = require('fc')(render, 1)
const camera = require('ctx-camera')(ctx, window, {})


const out = vec2.create()
const fov = Math.PI/4
const eye = [0, 0]
const target = [1, 0]
const movementSpeed = .5

function render() {
  stock.rotation += 0.01
  stock.pos[0] = -25
  //stock.pos[1] = -25

  if (keyboard.keys['w']) {
    eye[1] += movementSpeed
  }
  if (keyboard.keys['s']) {
    eye[1] -= movementSpeed
  }

  if (keyboard.keys['a']) {
    eye[0] -= movementSpeed
  }
  if (keyboard.keys['d']) {
    eye[0] += movementSpeed
  }


  volumes[0].pos = [Math.sin(Date.now() / 1000) * 200, Math.cos(Date.now() / 1000) * 25]
  stock.scale = [Math.abs(Math.sin(Date.now() / 1000) + 2) * 50, 1.0]
  ctx.clear()
  console.clear()
  camera.begin()
    center(ctx)
    ctx.scale(10, -10)
    ctx.lineCap = "round"
    ctx.lineWidth= .2
    ctx.pointToWorld(target, camera.mouse.pos)
    // Render cascades from course to fine
    for (var i=cascadeCount-1; i>=0; i--) {
      cascades[i].center = [
        eye[0],
        eye[1]
      ]
      cascades[i].reset()
      cascades[i].render(ctx)

      // TODO: consider using the previous, coarser, cascade as a source of data
      volumes.forEach((volume) => {
        cascades[i].addVolume(ctx, volume.modelMatrix, volume)
      })
    }

    volumes.forEach((volume) => volume.render(ctx))



    var view = mat3.create()
    cameraLookAt(view, eye, target)
    drawCameraRays(ctx, fov, view, mat3.create(), eye, target, (ray) => {
      ctx.beginPath()
      ctx.moveTo(eye[0], eye[1])
      ctx.lineTo(
        eye[0] + ray.dir[0] * 1000,
        eye[1] + ray.dir[1] * 1000
      )
      ctx.strokeStyle = 'white'
      ctx.stroke()
    })


    ctx.save()
    ctx.scale(.25, .25)
    ctx.fillStyle = "#999"
    drawCamera(ctx, eye, target)
    ctx.restore()

  camera.end()
}

