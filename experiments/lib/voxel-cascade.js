const ndarray = require('ndarray')
const hsl = require('./hsl')
const {vec2} = require('gl-matrix')
const renderGrid = require('ctx-render-grid-lines')
const collide = require('./collision/aabb-obb')
const offsets = [
  [0, 1],
  [1, 1],
  [1, 0],
  [0, 0]
]

module.exports = createCascade

const min = Math.min
const max = Math.max
const floor = Math.floor
const ceil = Math.ceil
const out = vec2.create()

function clamp(l, u, v) {
  return min(u, max(v, l))
}

function createCascade(level, cascadeDiameter, cascadeCount) {
  const ret = {
    grid: ndarray([], [cascadeDiameter, cascadeDiameter]),
    cellSize: Math.pow(2, level + 1),
    prevCellSize: Math.pow(2, level),
    percent: level/(cascadeCount + 1),
    color: hsl(level/(cascadeCount + 1)),
    center: [0, 0],
    diameter: cascadeDiameter,
    radius: cascadeDiameter/2
  }

  ret.render = function(ctx) {

    var cur = this.cellSize * cascadeDiameter/2
    var cx = this.center[0]
    var cy = this.center[1]


    // ctx.beginPath()
    //   ctx.moveTo(cx-cur, cy-cur)
    //   ctx.lineTo(cx-cur, cy+cur)
    //   ctx.lineTo(cx+cur, cy+cur)
    //   ctx.lineTo(cx+cur, cy-cur)
    //   ctx.lineTo(cx-cur, cy-cur)
    //   ctx.strokeStyle=this.color
    //   ctx.stroke()

    ctx.beginPath()
      renderGrid(ctx, this.cellSize,
        this.center[0] - cur,
        this.center[1] - cur,
        this.center[0] + cur,
        this.center[1] + cur
      )
      ctx.strokeStyle = "#666"

      ctx.stroke()
  }

  ret.reset = function() {
    this.grid = ndarray([], [cascadeDiameter, cascadeDiameter])
  }

  ret.addBrick = function(cell, brick) {
    const o = this.grid.get(cell[0], cell[1])
    if (!o) {
      this.grid.set(cell[0], cell[1], [brick])
      return
    }
    o.push(brick)
  }

  ret.addVolume = function(ctx, mat, volume) {
    const cascadePos = [0, 0]
    const verts = [
      vec2.create(),
      vec2.create(),
      vec2.create(),
      vec2.create()
    ]
    const lower = [Number.MAX_VALUE, Number.MAX_VALUE]
    const upper = [-Number.MAX_VALUE, -Number.MAX_VALUE]
    const start = vec2.create()
    const end = vec2.create()



    function cellToWorld(v, cellSize) {
      return v*cellSize
    }
    const radius = cascadeDiameter/2

    volume.bricks.forEach((brick) => {
      const txVerts = verts.map((vert, i) => {
        const out = vec2.create()
        vec2.transformMat3(
          out,
          vec2.add(out, brick.index, offsets[i]),
          mat
        )

        lower[0] = min(lower[0], out[0] - this.center[0])
        upper[0] = max(upper[0], out[0] - this.center[0])

        lower[1] = min(lower[1], out[1] - this.center[1])
        upper[1] = max(upper[1], out[1] - this.center[1])

        return out
      })

      // compute the index space bounding box
      start[0] = clamp(-radius, radius, floor(lower[0]/this.cellSize))
      start[1] = clamp(-radius, radius, floor(lower[1]/this.cellSize))
      end[0] = clamp(-radius, radius, ceil(upper[0]/this.cellSize))
      end[1] = clamp(-radius, radius, ceil(upper[1]/this.cellSize))

      if (end[0] - start[0] < 1 || end[1] - start[1] < 1) {
        return
      }

      // iterate over the bounding box
      for (var x = start[0]; x < end[0]; x += 1) {
        for (var y = start[1]; y < end[1]; y += 1) {
          cascadePos[0] = (x | 0) * this.cellSize
          cascadePos[1] = (y | 0) * this.cellSize

          const p = [
            [
              this.center[0] + cascadePos[0],
              this.center[1] + cascadePos[1]
            ],
            [
              this.center[0] + cascadePos[0] + this.cellSize,
              this.center[1] + cascadePos[1] + this.cellSize
            ]
          ];

          const np = [[0, 0], [0, 0]]
          vec2.normalize(np[0], p[0])
          vec2.normalize(np[1], p[1])

          if (collide(p, txVerts)) {
            this.addBrick([x + this.radius, y + this.radius], brick)

            if (ctx) {
              ctx.strokeStyle = this.color
              ctx.strokeRect(
                p[0][0],
                p[0][1],
                p[1][0] - p[0][0],
                p[1][1] - p[0][1]
              )
            }
          }
        }
      }
    })
  }

  return ret
}
