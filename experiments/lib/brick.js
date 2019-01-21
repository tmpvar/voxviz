const ndarray = require("ndarray")
const fill = require('ndarray-fill')
const square = require('./square')
const BRICK_DIAMETER = 4
const BRICK_RADIUS = BRICK_DIAMETER / 2
const BRICK_DIAMETER_SQUARED = Math.pow(BRICK_DIAMETER, 2)
const INV_BRICK_DIAMETER = 1.0 / BRICK_DIAMETER
const { mat3, vec2 } = require('gl-matrix')
const inVec = [0, 0]

const txo = require('./txo')

const min = Math.min
const max = Math.max
const floor = Math.floor
const ceil = Math.ceil
const sign = Math.sign
const abs = Math.abs

class Brick {
  constructor(index, volume) {
    this.volume = volume
    this.index = [index[0], index[1]]
    this.grid = ndarray(new Float32Array(BRICK_DIAMETER_SQUARED), [BRICK_DIAMETER, BRICK_DIAMETER])
    this.empty = true
  }

  render(ctx, mat) {
    for (var x = 0; x<this.grid.shape[0]; x++) {
      inVec[0] = x
      for (var y = 0; y<this.grid.shape[1]; y++) {
        inVec[1] = y

        var d = this.grid.get(x, y)
        ctx.beginPath()
        if (d > 0) {
          ctx.fillStyle = `hsla(${(d - 1) * 360}, 100%, 56%, 0.5)`

          square(
            ctx,
            mat,
            this.index[0] + x * INV_BRICK_DIAMETER,
            this.index[1] + y * INV_BRICK_DIAMETER,
            INV_BRICK_DIAMETER,
            INV_BRICK_DIAMETER
          )
          ctx.fill()
        }

      }
    }
  }

  fill(fn) {
    fill(this.grid, fn)
    return this
  }

  dda(ray, fn) {
    const v2tmp = vec2.create()
    const grid = this.grid

    const pos = [
      ray.origin[0] * BRICK_DIAMETER,
      ray.origin[1] * BRICK_DIAMETER
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
      (rayStep[0] * (mapPos[0] - pos[0]) + (rayStep[0] * 0.5) + 0.5) * deltaDist[0],
      (rayStep[1] * (mapPos[1] - pos[1]) + (rayStep[1] * 0.5) + 0.5) * deltaDist[1]
    ]
    const mask = [0, 0]
    mask[0] = (sideDist[0] <= sideDist[1]) | 0
    mask[1] = (sideDist[1] <= sideDist[0]) | 0

    // dda
    for (var i = 0.0; i < 16; i++ ) {
      var indexPosX = mapPos[0]
      var indexPosY = mapPos[1]
      if (indexPosX >= 0 &&
          indexPosX < grid.shape[0] &&
          indexPosY >= 0 &&
          indexPosY < grid.shape[1]
      )
      {
        var value = grid.get(indexPosX, indexPosY)
        if (fn && fn([indexPosX, indexPosY], value)) {
          return true;
        }
      }

      mask[0] = (sideDist[0] <= sideDist[1]) | 0
      mask[1] = (sideDist[1] <= sideDist[0]) | 0

      sideDist[0] += mask[0] * deltaDist[0]
      sideDist[1] += mask[1] * deltaDist[1]

      mapPos[0] += mask[0] * rayStep[0],
      mapPos[1] += mask[1] * rayStep[1]
    }
    return false
  }

  transformRay(ray) {
    var volume = this.volume
    const invModel = mat3.create()
    mat3.translate(invModel, volume.modelMatrix, this.index)
    mat3.invert(invModel, invModel)
    var invOrigin = txo([0, 0], invModel, ray.origin)
    var txTarget = txo([0, 0], invModel, [
      ray.origin[0] + ray.dir[0],
      ray.origin[1] + ray.dir[1]
    ])

    var txDir = [
      txTarget[0] - invOrigin[0],
      txTarget[1] - invOrigin[1]
    ]

    var invDir = [
      1 / txDir[0],
      1 / txDir[1]
    ]

    const txRay = {
      origin: invOrigin,
      dir: txDir,
      invDir: invDir,
      nDir: vec2.normalize([0, 0], txDir)
    }
    return txRay
  }
}

module.exports = {
  Brick: Brick,
  BRICK_DIAMETER: BRICK_DIAMETER,
  BRICK_RADIUS: BRICK_RADIUS
}
