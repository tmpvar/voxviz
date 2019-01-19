
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
      // if (idx != 8) { return }

      const dist = marchGrid(ctx, cascades, ray)

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


function rayCell(ctx, center, target, cell, ray) {
  if (!cell) {
    return
  }
  var d = Infinity
  const invModel = mat3.create()
  const origin = vec2.create()

  for (var i=0; i<cell.length; i++) {
    var brick = cell[i]
    var volume = brick.volume
    mat3.invert(invModel, volume.modelMatrix)
    var invOrigin = txo([0, 0], invModel, ray.origin)
    var invTarget = txo([0, 0], invModel, target)
    var invDir = [
      invTarget[0] - invOrigin[0],
      invTarget[1] - invOrigin[1]
    ]

    vec2.normalize(invDir, invDir)
    invDir[0] = 1 / invDir[0]
    invDir[1] = 1 / invDir[1]


    var aabb = [
      brick.index,
      [
        brick.index[0] + 1,
        brick.index[1] + 1
      ]
    ]
    var out = [0, 0]
    ctx.beginPath()
      ctx.strokeStyle = "yellow"
      ctx.moveTo(invOrigin[0], invOrigin[1])
      ctx.lineTo(invTarget[0], invTarget[1])
      // ctx.moveTo(center[0], center[1])
      // ctx.lineTo(target[0], target[1])
      ctx.stroke()

    if (raySlab(invOrigin, invDir, aabb, out)) {
      d = Math.min(vec2.distance(ray.origin, out), d)
    }

  }
  return d
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
  for (var i = 0.0; i < 64; i++ ) {
    var indexPosX = mapPos[0] + cascade.radius
    var indexPosY = mapPos[1] + cascade.radius
    if (indexPosX < 0 ||
        indexPosX >= grid.shape[0] ||
        indexPosY < 0 ||
        indexPosY >= grid.shape[1]
    )
    {
      return 10000;
    }

    var cell = grid.get(indexPosX, indexPosY)
    if (cell) {
      return rayCell(ctx, center, pos, cell, ray)

      // ctx.strokeStyle = "green"
      // ctx.strokeRect(
      //   center[0] + mapPos[0] * cellSize + 0.5,
      //   center[1] + mapPos[1] * cellSize + 0.5,
      //   cellSize-1,
      //   cellSize-1
      // )
      // return vec2.length([mapPos[0] * cellSize, mapPos[1] * cellSize])
      return
    } else {
      ctx.strokeStyle = cascade.color
      ctx.strokeRect(
        center[0] + mapPos[0] * cellSize + .5,
        center[1] + mapPos[1] * cellSize + .5,
        cellSize-1,
        cellSize-1
      )
    }

    mask[0] = (sideDist[0] <= sideDist[1]) | 0
    mask[1] = (sideDist[1] <= sideDist[0]) | 0

    sideDist[0] += mask[0] * deltaDist[0]
    sideDist[1] += mask[1] * deltaDist[1]

    mapPos[0] += mask[0] * rayStep[0]
    mapPos[1] += mask[1] * rayStep[1]

    pos[0] += mask[0] * ray.dir[0] * cellSize
    pos[1] += mask[1] * ray.dir[1] * cellSize
  }
  return 10000
}

