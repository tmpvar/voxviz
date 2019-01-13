
const center = require('ctx-translate-center')
const glmatrix = require('gl-matrix')
const vec2 = glmatrix.vec2
const mat3 = glmatrix.mat3
const segseg = require('segseg')
const raySlab = require('ray-aabb-slab')
const Volume = require('./lib/volume')
const { BRICK_DIAMETER, BRICK_RADIUS } = require('./lib/brick')
const createCascade = require('./lib/voxel-cascade')
const cameraLookAt = require('./lib/lookat')
const drawCamera = require('./lib/render/camera')
const drawCameraRays = require('./lib/render/camera-rays')
const hsl = require('./lib/hsl')
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
const sign = Math.sign
const abs = Math.abs

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
const eye = [0, -20]
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


  //volumes[0].pos = [Math.sin(Date.now() / 1000) * 200, Math.cos(Date.now() / 1000) * 25]
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
    }

    for (var i=0; i<cascadeCount; i++) {
      // TODO: consider using the previous, coarser, cascade as a source of data
      volumes.forEach((volume) => {
        cascades[i].addVolume(ctx, volume.modelMatrix, volume)
      })
    }

    volumes.forEach((volume) => volume.render(ctx))



    var view = mat3.create()
    cameraLookAt(view, eye, target)

    ctx.save()
    ctx.lineWidth = .15
    drawCameraRays(ctx, fov, view, mat3.create(), eye, target, (ray, idx, total) => {
      if (idx != 8) { return }

      marchGrid(ctx, cascades, ray)

      ctx.beginPath()
      ctx.moveTo(eye[0], eye[1])
      ctx.lineTo(
        eye[0] + ray.dir[0] * 1000,
        eye[1] + ray.dir[1] * 1000
      )
      ctx.strokeStyle = hsl(idx/(total + 1))
      ctx.stroke()

    })
    ctx.restore()

    ctx.fillStyle = "#999"
    drawCamera(ctx, eye, target, .25)

  camera.end()
}

function step(a, b) {
  return b < a ? 0.0 : 1.0
}

function marchGrid(ctx, cascades, ray) {
  const v2tmp = vec2.create()
  const cascade = cascades[cascades.length-3]
  const grid = cascade.grid
  const center = cascade.center
  const cellSize = cascade.cellSize
  const pos = [
    ray.dir[0] * 0.01,
    ray.dir[1] * 0.01
  ]

  const mapPos = [
    floor(pos[0]),
    floor(pos[1])
  ]

  var length = vec2.length(ray.dir)
  const deltaDist = [
    abs(length / ray.dir[0]),
    abs(length / ray.dir[1])
  ]

  const rayStep = [
    sign(ray.dir[0]),
    sign(ray.dir[1])
  ]

  const sideDist = [
    (sign(ray.dir[0]) * (mapPos[0] - pos[0]) + (sign(ray.dir[0]) * 0.5) + 0.5) * deltaDist[0],
    (sign(ray.dir[1]) * (mapPos[1] - pos[1]) + (sign(ray.dir[1]) * 0.5) + 0.5) * deltaDist[1]
  ]
  const mask = [0, 0]

  // // dda
  for (var i = 0.0; i < 64; i++ ) {
    var indexPosX = mapPos[0] + cascade.radius
    var indexPosY = mapPos[1] + cascade.radius

    if (indexPosX < 0 ||
        indexPosX >= grid.shape[0] ||
        indexPosY < 0 ||
        indexPosY >= grid.shape[1]
    )
    {
      return false;
    }

    if (grid.get(indexPosX, indexPosY)) {
      ctx.fillStyle = "green"
      ctx.fillRect(
        center[0] + mapPos[0] * cellSize + 1,
        center[1] + mapPos[1] * cellSize + 1,
        cellSize-2,
        cellSize-2
      )
      return true
    } else {
      ctx.fillStyle = cascade.color
      ctx.fillRect(
        center[0] + mapPos[0] * cellSize + 1,
        center[1] + mapPos[1] * cellSize + 1,
        cellSize-2,
        cellSize-2
      )
    }

    mask[0] = (sideDist[0] <= sideDist[1]) | 0
    mask[1] = (sideDist[1] <= sideDist[0]) | 0

    sideDist[0] += mask[0] * deltaDist[0]
    sideDist[1] += mask[1] * deltaDist[1]

    mapPos[0] += mask[0] * rayStep[0]
    mapPos[1] += mask[1] * rayStep[1]
  }
  return false
}

