const ndarray = require("ndarray")
const fill = require('ndarray-fill')
const square = require('./square')
const BRICK_DIAMETER = 4
const BRICK_RADIUS = BRICK_DIAMETER / 2
const BRICK_DIAMETER_SQUARED = Math.pow(BRICK_DIAMETER, 2)
const INV_BRICK_DIAMETER = 1.0 / BRICK_DIAMETER

const inVec = [0, 0]


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
}

module.exports = {
  Brick: Brick,
  BRICK_DIAMETER: BRICK_DIAMETER,
  BRICK_RADIUS: BRICK_RADIUS
}
