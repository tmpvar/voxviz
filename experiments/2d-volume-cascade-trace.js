
const ctxCenter = require('ctx-translate-center')
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
const txo = require('./lib/txo')
const square = require('./lib/square')
const drawCameraRays = require('./lib/render/camera-rays')
const hsl = require('./lib/hsl')
const createKeyboard = require('./lib/keyboard')
const keyboard = createKeyboard()
const cascadeDiameter = 16
const cascadeCount = 5
const cascades = []
for (var i = 0; i<cascadeCount; i++) {
  cascades.push(createCascade(i, cascadeDiameter, cascadeCount))
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
  return x%2 == 0 && y%2 === 0 ? v : 0;
}).empty = false
stock.scale = [10, 10]

const longOne = new Volume([10, 0])
longOne.addBrick([0, 0]).fill((x, y) => {
   return 1;
}).empty = false
longOne.scale = [10, 1]

const volumes = [stock, longOne]


const ctx = require('fc')(render, 1)
const camera = require('ctx-camera')(ctx, window, {})


const out = vec2.create()
const fov = Math.PI/4
const eye = [0, -20]
const target = [1, 0]
const movementSpeed = .5

function render() {
  stock.rotation += 0.01
  //stock.pos[0] = -25
  //stock.pos[1] = -25
  stock.pos = [Math.sin(Date.now() / 1000) * 20, Math.cos(Date.now() / 1000) * 25]
  longOne.scale = [Math.abs(Math.sin(Date.now() / 1000) + 2) * 50, 1.0]

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


  ctx.clear()
  console.clear()
  camera.begin()
    ctxCenter(ctx)
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

    volumes.forEach((volume) => volume.render(ctx))

    for (var i=0; i<cascadeCount; i++) {
      // TODO: consider using the previous, coarser, cascade as a source of data
      volumes.forEach((volume) => {
        cascades[i].addVolume(ctx, volume.modelMatrix, volume)
      })
    }

    var view = mat3.create()
    cameraLookAt(view, eye, target)

    var model = mat3.create()
    mat3.translate(model, model, eye)

    ctx.save()
    ctx.lineWidth = .15
    drawCameraRays(fov, eye, target, (ray, idx, total) => {
      ctx.beginPath()
      ctx.moveTo(eye[0], eye[1])
      ctx.lineTo(
        eye[0] + ray.dir[0] * 1000,
        eye[1] + ray.dir[1] * 1000
      )
      ctx.strokeStyle = "#444"
      ctx.stroke()

      const dist = marchGrid(ctx, cascades, ray)
      if (!isFinite(dist)) {
        return
      }
      ctx.beginPath()
      ctx.moveTo(eye[0], eye[1])
      ctx.lineTo(
        eye[0] + ray.dir[0] * dist,
        eye[1] + ray.dir[1] * dist
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
  const cascade = cascades[2]
  const grid = cascade.grid
  const center = cascade.center
  const cellSize = cascade.cellSize
  const pos = [
    ray.dir[0] * 0.00001,
    ray.dir[1] * 0.00001
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
  for (var i = 0.0; i < 16; i++ ) {
    var indexPosX = mapPos[0] + cascade.radius
    var indexPosY = mapPos[1] + cascade.radius
    if (indexPosX < 0 ||
        indexPosX >= grid.shape[0] ||
        indexPosY < 0 ||
        indexPosY >= grid.shape[1]
    )
    {
      return 0;
    }

    ctx.strokeStyle = cascade.color
    ctx.strokeRect(
      center[0] + mapPos[0] * cellSize + .5,
      center[1] + mapPos[1] * cellSize + .5,
      cellSize-1,
      cellSize-1
    )

    var cell = grid.get(indexPosX, indexPosY)
    if (cell) {
      var d = rayCell(ctx, center, cell, ray)
      if (isFinite(d)) {
        return d
      }
    }

    mask[0] = (sideDist[0] <= sideDist[1]) | 0
    mask[1] = (sideDist[1] <= sideDist[0]) | 0

    sideDist[0] += mask[0] * deltaDist[0]
    sideDist[1] += mask[1] * deltaDist[1]

    mapPos[0] += mask[0] * rayStep[0],
    mapPos[1] += mask[1] * rayStep[1]
  }
  return 0
}

function rayCell(ctx, center, cell, ray) {
  var d = Infinity
  if (!cell) {
    return d
  }

  for (var i=0; i<cell.length; i++) {
    var brick = cell[i]
    var aabb = [
      brick.index,
      [
        brick.index[0] + 1,
        brick.index[1] + 1
      ]
    ]

    const txRay = brick.transformRay(ray)

    var out = [0, 0]
    ctx.beginPath()
    if (raySlab(txRay.origin, txRay.invDir, aabb, out)) {
      if (out[0] < 0) {
        return d
      }

      txRay.origin[0] += txRay.dir[0] * out[0]
      txRay.origin[1] += txRay.dir[1] * out[0]

      brick.dda(txRay, (pos, value) => {

        ctx.beginPath()
        ctx.strokeStyle = value ? "green" : "red"
        square(
          ctx,
          brick.volume.modelMatrix,
          (pos[0])/BRICK_DIAMETER,
          (pos[1])/BRICK_DIAMETER,
          1/BRICK_DIAMETER,
          1/BRICK_DIAMETER
        )
        ctx.closePath()
        ctx.stroke()
        return !!value
      })


      d = Math.min(out[0], d)
    }
  }
  return d
}