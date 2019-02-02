/*
TODO:
- traverse the cascade
- bounce rays off of voxels (cascade->brick->voxel->bounce->brick->cascade) which may be tricky
  because the rays are marched in brick space and we'll need to convert them back to world space.
- [optimization] bricks that overlap multiple cascade cells (on the same level) should move
  the ray origin to the cell edge prior to dda.
*/


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

const paused = 1

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

const ctx = require('fc')(render, !paused)
const camera = require('ctx-camera')(ctx, window, {})


const out = vec2.create()
const fov = Math.PI/4
const eye = [20, 25]
const target = [5, 00]
const movementSpeed = .5

//stock.rotation += 0.7
stock.pos[0] = 10


function render() {
  //stock.rotation += 0.01
  //stock.pos[0] = -25
  //stock.pos[1] = -25
  //stock.pos = [Math.sin(Date.now() / 1000) * 20, Math.cos(Date.now() / 1000) * 25]
  //longOne.scale = [Math.abs(Math.sin(Date.now() / 1000) + 2) * 50, 1.0]

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
    !paused && ctx.pointToWorld(target, camera.mouse.pos)
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
        cascades[i].addVolume(null, volume.modelMatrix, volume)
      })
    }

    var view = mat3.create()
    cameraLookAt(view, eye, target)

    var model = mat3.create()
    mat3.translate(model, model, eye)

    ctx.save()
    ctx.lineWidth = .15
    drawCameraRays(fov, eye, target, (ray, idx, total) => {
      if (idx != 32) return
      ray.level = cascadeCount - 1

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

function insideCascadeRadius(pos, cascade, isLower) {
  if (!cascade) {
    return false
  }

  const x = isLower ? shift(pos[0], 1) : pos[0]
  const y = isLower ? shift(pos[1], 1) : pos[1]

  return abs(x) < cascade.radius && abs(y) < cascade.radius
}

function shift(x, dir) {
  const s = sign(x)
  const n = dir < 0 ? (Math.abs(x) >> 1) : (Math.abs(x) << 1)
  return s * n
}

function moveIndexPos(pos, dir) {
  pos[0] = shift(pos[0], dir)
  pos[1] = shift(pos[1], dir)
}

function marchGrid(ctx, cascades, ray) {
  const v2tmp = vec2.create()
  const pos = [
    ray.dir[0] * 0.00001,
    ray.dir[1] * 0.00001
  ]
  const mapPos = [
    floor(ray.dir[0]),
    floor(ray.dir[1])
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

  var t = 0

  const sideDist = [
    (rayStep[0] * (mapPos[0] - ray.dir[0]) + (rayStep[0] * 0.5) + 0.5) * deltaDist[0],
    (rayStep[1] * (mapPos[1] - ray.dir[1]) + (rayStep[1] * 0.5) + 0.5) * deltaDist[1]
  ]
  const mask = [0, 0]

  // dda
  const maxSteps = cascades.length*16
  for (var i = 0.0; i < maxSteps; i++ ) {

    var cascade = cascades[ray.level]
    var lowerCascade = cascades[ray.level-1]
    var upperCascade = cascades[ray.level+1]

    ctx.beginPath()
    ctx.arc(
      cascade.center[0] + pos[0],
      cascade.center[1] + pos[1],
      .5 * ray.level,
      0,
      Math.PI*2
    )
    ctx.strokeStyle = cascade.color
    ctx.stroke()


    var indexPosX = mapPos[0] + cascade.radius
    var indexPosY = mapPos[1] + cascade.radius
    if (indexPosX < 0 ||
        indexPosX >= cascade.grid.shape[0] ||
        indexPosY < 0 ||
        indexPosY >= cascade.grid.shape[1]
    )
    {
      ctx.strokeStyle = 'red'
      ctx.strokeRect(
        cascade.center[0] + mapPos[0] * cascade.cellSize + .5,
        cascade.center[1] + mapPos[1] * cascade.cellSize + .5,
        cascade.cellSize-1,
        cascade.cellSize-1
      )
      return 0;
    }

    ctx.strokeStyle = cascade.color
    ctx.strokeRect(
      cascade.center[0] + mapPos[0] * cascade.cellSize + .5,
      cascade.center[1] + mapPos[1] * cascade.cellSize + .5,
      cascade.cellSize-1,
      cascade.cellSize-1
    )


    var currentValue = cascade.grid.get(indexPosX, indexPosY)
    var upperValue = upperCascade && upperCascade.grid.get(
      // shift(mapPos[0], -1) + upperCascade.radius,
      // shift(mapPos[1], -1) + upperCascade.radius
      floor(pos[0] / upperCascade.cellSize) + upperCascade.radius,
      floor(pos[1] / upperCascade.cellSize) + upperCascade.radius
    )

    var canGoDown = ray.level > 0 && insideCascadeRadius(mapPos, lowerCascade, true) && !!currentValue
    var canGoUp = !currentValue && upperCascade && !upperValue
    //var canStep = insideCascadeRadius(mapPos, cascade, false)
    var levelChange = 0
    if (canGoDown) {
      levelChange = -1
    } else if (canGoUp) {
      levelChange = 1
    }

    console.group('cycle (level: ' + ray.level + ')')
    console.log('up: %s, down: %s, current: %s, upper: %s', canGoUp, canGoDown, !!currentValue, !!upperValue)
    console.log('pos(%s, %s); mapPos(%s, %s)', f(pos[0]), f(pos[1]), f(mapPos[0]), f(mapPos[1]))

    if (levelChange !== 0) {
      console.group('level')
      console.log('level %s (%s); mapPos(%s, %s); pos(%s, %s)', ray.level,  levelChange > 0 ? '^' : 'v', mapPos[0], mapPos[1], f(pos[0]), f(pos[1]))
      ray.level += levelChange
      mapPos[0] = floor(pos[0] / cascades[ray.level].cellSize)
      mapPos[1] = floor(pos[1] / cascades[ray.level].cellSize)
      console.log('level %s; mapPos(%s, %s); pos(%s, %s)', ray.level, mapPos[0], mapPos[1], f(pos[0]), f(pos[1]))

      console.groupEnd()
      console.groupEnd()
      continue
    }


    if (currentValue) {
      var closest = rayCell(ctx, cascade.center, currentValue, ray)

      if (closest.terminated) {
        console.log('ray terminated')
        console.groupEnd()
        return closest.tmin
      }
    }

    console.group('step')
    console.log('STEP pos(%s, %s); mapPos(%s, %s); level:%s', f(pos[0]), f(pos[1]), f(mapPos[0]), f(mapPos[1]), ray.level)
    mask[0] = (sideDist[0] <= sideDist[1]) | 0
    mask[1] = (sideDist[1] <= sideDist[0]) | 0

    sideDist[0] += mask[0] * deltaDist[0]
    sideDist[1] += mask[1] * deltaDist[1]

    mapPos[0] += mask[0] * rayStep[0]
    mapPos[1] += mask[1] * rayStep[1]

    var lt = min(deltaDist[0], deltaDist[1]) * cascade.cellSize

    pos[0] += ray.dir[0] * lt
    pos[1] += ray.dir[1] * lt

    console.log('STEP pos(%s, %s); mapPos(%s, %s); level:%s', f(pos[0]), f(pos[1]), f(mapPos[0]), f(mapPos[1]), ray.level)
    console.groupEnd()
    console.groupEnd()

  }
  return 0
}

function f(x) {
  return Number(x).toFixed(2)
}

function rayCell(ctx, center, cell, ray) {
  const closest = {
    tmin: Infinity,
    tmax: -Infinity,
    brick: null,
    terminated: false
  }

  if (!cell) {
    return closest
  }

  const aabb = [[0, 0], [0, 0]]

  for (var i=0; i<cell.length; i++) {
    var brick = cell[i]
    aabb[0][0] = brick.index[0]
    aabb[0][1] = brick.index[1]
    aabb[1][0] = brick.index[0] + 1
    aabb[1][1] = brick.index[1] + 1

    const txRay = brick.transformRay(ray)

    var out = [0, 0]
    ctx.beginPath()
    if (raySlab(txRay.origin, txRay.invDir, aabb, out)) {
      // FIXME: we should be able to trace rays from inside of a brick.
      if (out[0] < 0) {
        return closest
      }

      txRay.origin[0] += txRay.dir[0] * out[0]
      txRay.origin[1] += txRay.dir[1] * out[0]

      // TODO: support partial hits / accumulation
      var hit = brick.dda(txRay, (pos, value) => {
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

      if (closest.tmin > out[0]) {
        closest.tmin = out[0]
        closest.brick = brick
      }

      if (closest.tmax < out[1]) {
        closest.tmax = out[1]
      }

      if (hit) {
        closest.terminated = true
        return closest
      }
    }
  }
  return closest
}