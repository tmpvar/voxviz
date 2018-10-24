const ctx = require('fc')(render, 1)
const center = require('ctx-translate-center')
const glmatrix = require('gl-matrix')
const renderGrid = require('ctx-render-grid-lines')
const vec2 = glmatrix.vec2
const mat3 = glmatrix.mat3
const segseg = require('segseg')
const ndarray = require('ndarray')
const raySlab = require('ray-aabb-slab')
const camera = require('ctx-camera')(ctx, window, {})

const grid = ndarray(new Float32Array(265), [16, 16])
const cellRadius = 5

const brickAABB = [
  [-8 * cellRadius, -8 * cellRadius],
  [ 8 * cellRadius,  8 * cellRadius]
]

const xfBrickAABB = [
  [0, 0],
  [0, 0]
]


const lineTo = ctx.lineTo.bind(ctx)
const moveTo = ctx.moveTo.bind(ctx)
const translate = ctx.translate.bind(ctx)
const floor = Math.floor

const view = mat3.create()
const model = mat3.create()
const invModel = mat3.create()
const v2tmp = vec2.create()
const v2tmp2 = vec2.create()
const v2dir = vec2.create()
const v2offset = vec2.create()
const v2gridPos = vec2.create()

fillBrick()


// camera
const v2tmpCam = vec2.create()
const eye = [-100, 0]
const origin = [0, 0]
function cameraLookAt(mat, eye, target) {
  vec2.subtract(v2tmpCam, target, eye)
  mat3.identity(mat)

  const rot = Math.atan2(v2tmpCam[1], v2tmpCam[0])
  mat3.translate(mat, mat, eye)
  mat3.rotate(mat, mat, rot)
}

// TODO: locate active cell + dda through the grid

function render() {
  const now = Date.now() /10
  vec2.set(eye,
    Math.sin(now / 1000) * 200,
    Math.cos(now / 1000) * 200
  )
  mat3.identity(model)
  var scale =  [1 + Math.abs(Math.sin(now / 100)), 1 + Math.abs(Math.cos(now / 1000))]

  //mat3.scale(model, model, scale)
  mat3.translate(model, model, [Math.sin(now / 100) * cellRadius * 8 - cellRadius * 60, 0.0])

  mat3.rotate(model, model, now / 100)



  // render prereqs
  mat3.invert(invModel, model)
  var invEye = txo([0, 0], invModel, eye)
  var txOrigin = txo([0, 0], model, origin)

    ctx.clear()
    camera.begin()
      center(ctx)
      ctx.scale(2.0, 2.0)

      ctx.save()
        ctx.lineWidth = 0.05
        ctx.beginPath()
          renderGrid(
            ctx,
            cellRadius,
            -cellRadius * 2 * 50,
            -cellRadius * 2 * 50,
            cellRadius * 2 * 50,
            cellRadius * 2 * 50
          )
          ctx.strokeStyle = '#AAA'
          ctx.stroke()

        ctx.lineWidth += 0.7
        ctx.strokeStyle = "#aaa"
        ctx.beginPath()
        // draw top line
        tx(model, -grid.shape[0]/2 * cellRadius, -grid.shape[1]/2 * cellRadius - 10, moveTo)
        tx(model, grid.shape[0]/2 * cellRadius, -grid.shape[1]/2 * cellRadius - 10, lineTo)
        ctx.moveTo(-grid.shape[0]/2 * cellRadius, -grid.shape[1]/2 * cellRadius - 10)
        ctx.lineTo(grid.shape[0]/2 * cellRadius, -grid.shape[1]/2 * cellRadius - 10)
        // draw original shape oriented around origin
        square(
          model,
          -grid.shape[0]/2 * cellRadius,
          -grid.shape[1]/2 * cellRadius,
          grid.shape[0] * cellRadius,
          grid.shape[0] * cellRadius
        )
        ctx.stroke()

        ctx.strokeStyle = "#aaa"
        ctx.strokeRect(
          -grid.shape[0]/2 * cellRadius,
          -grid.shape[1]/2 * cellRadius,
          grid.shape[0] * cellRadius,
          grid.shape[0] * cellRadius
        )

      ctx.restore()

      // transformed render
      drawBrick(model)
      ctx.strokeStyle = "#4eb721"
      ctx.fillStyle = "#367c17"
      cameraLookAt(view, eye, txOrigin)
      drawCameraRays(view, model, eye, txOrigin, false)
      drawCamera(eye, txOrigin)

      // original render
      drawBrick(mat3.create())
      ctx.fillStyle = ctx.strokeStyle = "#f0f"
      drawCameraRays(view, mat3.create(), invEye, origin, true)
      drawCamera(invEye, origin)

      //trace()
    camera.end();
}

function hsl(p, a) {
  return `hsla(${p*360}, 100%, 46%, ${a||1})`
}

function drawCameraRays(viewMat, modelMat, eye, origin, doMarch) {
  var dx = origin[0] - eye[0]
  var dy = origin[1] - eye[1]
  var dir = [dx, dy]
  var nDir = vec2.normalize(vec2.create(), dir)

  var numRays = 64;
  var fov = Math.PI/16
  var start = -fov/2
  var step = fov/numRays
  var rayDir = vec2.fromValues(1, 0)
  var zero = [0, 0]
  var invRayDir = vec2.create()
  for (var i=0; i<numRays; i++) {
    vec2.rotate(rayDir, dir, zero, start + i * step)
    vec2.normalize(rayDir, rayDir)
    vec2.set(invRayDir, 1/rayDir[0], 1/rayDir[1])

    var res = [0, 0]
    raySlab(eye, invRayDir, brickAABB, res)
    ctx.beginPath()
      ctx.arc(
        eye[0] + rayDir[0] * res[0],
        eye[1] + rayDir[1] * res[0],
        1,
        0,
        Math.PI*2
      )
    ctx.stroke()

    if (!doMarch) {
        ctx.beginPath()
          ctx.moveTo(eye[0], eye[1])
          ctx.lineTo(rayDir[0] * 6000, rayDir[1] * 6000)
          ctx.stroke()
          continue

    }


    var isect = false
    // if the ray completely misses don't bother marching the grid
    if (isFinite(res[0]) && res[0]) {
      ctx.strokeStyle = hsl(i/numRays)
      if (doMarch) {
        vec2.set(v2gridPos, eye[0] + rayDir[0] * res[0], eye[1] + rayDir[1] * res[0])
        //ctx.save()
        isect = marchGrid(modelMat, v2gridPos, nDir)
        //ctx.restore()
      }

      ctx.beginPath()
        ctx.moveTo(eye[0], eye[1])
        ctx.lineTo(eye[0] + rayDir[0] * res[0], eye[1] + rayDir[1] * res[0],)
        ctx.stroke()
    }
  }
/*
  if (!isFinite(res[0]) || !isect) {
    ctx.strokeStyle = "#d65757"
    res[0] = 6000
    vec2.set(v2gridPos, eye[0] + v2dir[0] * res[0], eye[1] + v2dir[1] * res[0])
  } else {
    ctx.strokeStyle = "#4eb721"
  }
  */
}

function drawBrick(mat) {
  const inVec = [0, 0]
  const txVec = [0, 0]

  vec2.set(v2offset, brickAABB[0][0], brickAABB[0][1])
  for (var x = 0; x<grid.shape[0]; x++) {
    inVec[0] = x
    for (var y = 0; y<grid.shape[1]; y++) {
      inVec[1] = y

      var d = grid.get(x, y)
      // TODO: transform each coord
      ctx.beginPath()
      if (d > 0) {
        ctx.fillStyle = "#445"
      } else {
        ctx.fillStyle = "#aaa"
      }

      square(
        mat,
        brickAABB[0][0] + x*cellRadius+1,
        brickAABB[0][0] + y*cellRadius+1,
        cellRadius-2,
        cellRadius-2
      )
      ctx.fill()
    }
  }

  ctx.fillStyle = "#aaa"
  ctx.beginPath()
    txo(v2tmp, mat, [0, 0])
    ctx.arc(v2tmp[0], v2tmp[1], 1, 0, Math.PI*2, false)
    ctx.fill()
}

function fillBrick() {

  const inVec = [0, 0]
  const txVec = [0, 0]
  var r = grid.shape[0] / 2
  for (var x = 0; x<grid.shape[0]; x++) {
    inVec[0] = x
    for (var y = 0; y<grid.shape[1]; y++) {
      inVec[1] = y

      var d = vec2.length(
        vec2.set(
          v2tmp,
          (x - r),
          (y - r)
        )
      ) - r + 4 ;
      grid.set(x, y, d)

      // TODO: transform each coord
      ctx.fillStyle = "#d65757"
      ctx.fillRect(
        v2offset[0] + x*cellRadius+1,
        v2offset[1] + y*cellRadius+1,
        cellRadius-2,
        cellRadius-2
      )
    }
  }
}


function tx(mat, x, y, fn) {
    vec2.set(v2tmp, x, y)
    vec2.transformMat3(v2tmp, v2tmp, mat)
    fn(v2tmp[0], v2tmp[1])
}


function square(mat, x, y, w, h) {
  tx(mat, x, y, moveTo)
  tx(mat, x, y + h, lineTo)
  tx(mat, x + w, y + h, lineTo)
  tx(mat, x + w, y, lineTo)
  tx(mat, x, y, lineTo)
}

function txo(out, mat, vec) {
    vec2.transformMat3(out, vec, mat)
    return out
}


function drawCamera(eye, origin) {
  var a = mat3.create()
  cameraLookAt(a, eye, origin)

  ctx.save()
  ctx.beginPath()
    tx(a, -5, -5, moveTo)
    tx(a, +5, -5, lineTo)
    tx(a, +5, +5, lineTo)
    tx(a, -5, +5, lineTo)
    tx(a, -5, -5, lineTo)

    tx(a, +5,  +2, moveTo)
    tx(a, +10, +5, lineTo)
    tx(a, +10, -5, lineTo)
    tx(a, +5,  -2, lineTo)

    ctx.stroke()
    ctx.fill()
  ctx.restore()
}

const v2sign = vec2.create()
function sign(out, v) {
  return vec2.set(
    out,
    Math.sign(v[0]),
    Math.sign(v[1])
  )
}

function invert(v) {
  if (v == 0) {
    return 0
  }
  return 1.0 / v;
}

function marchGrid(mat, worldPos, dir) {
  ctx.lineWidth = 0.5

  var worldToBrick = [
    worldPos[0] - brickAABB[0][0],
    worldPos[1] - brickAABB[0][1]
  ]

  var gridPos = [
    floor((worldPos[0] - brickAABB[0][0]) / cellRadius + dir[0] * 0.5),
    floor((worldPos[1] - brickAABB[0][1]) / cellRadius + dir[1] * 0.5)
  ]

  var gridStep = sign(vec2.create(), sign(v2sign, dir))
  var invDir = [invert(dir[0]), invert(dir[1])]

  // grid space
  var corner = vec2.fromValues(
    Math.max(gridStep[0], 0.0),
    Math.max(gridStep[1], 0.0)
  )

  var mask = vec2.create();
  var ratio = vec2.create();
  vec2.set(
    ratio,
    (gridPos[0] - worldToBrick[0] / cellRadius) * invDir[0],
    (gridPos[1] - worldToBrick[1] / cellRadius) * invDir[1]
  )

  var ratioStep = vec2.multiply(vec2.create(), gridStep, invDir);

  // dda
  for (var i = 0.0; i < 64; i++ ) {
    if (gridPos[0] < 0 || gridPos[0] >= grid.shape[0] || gridPos[1] < 0 || gridPos[1] >= grid.shape[1]) {
      return false;
    }
    ctx.beginPath()
    if (grid.get(gridPos[0], gridPos[1]) <= 0.0) {
      square(
        mat,
        (brickAABB[0][0] + gridPos[0] * cellRadius + 1),
        (brickAABB[0][1] + gridPos[1] * cellRadius + 1),
        cellRadius-2,
        cellRadius-2
      )
      ctx.stroke()
      return true
    }

    square(
      mat,
      brickAABB[0][0] + gridPos[0] * cellRadius + 1,
      brickAABB[0][1] + gridPos[1] * cellRadius + 1,
      cellRadius-2,
      cellRadius-2
    )
    ctx.stroke()

    vec2.set(mask, (ratio[0] <= ratio[1])|0, (ratio[1] <= ratio[0])|0)
    vec2.add(gridPos, gridPos, vec2.multiply(v2tmp, gridStep, mask))
    vec2.add(ratio, ratio, vec2.multiply(v2tmp, ratioStep, mask))
  }

  return false
}
