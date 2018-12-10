
const center = require('ctx-translate-center')
const glmatrix = require('gl-matrix')
const renderGrid = require('ctx-render-grid-lines')
const vec2 = glmatrix.vec2
const mat3 = glmatrix.mat3
const segseg = require('segseg')
const ndarray = require('ndarray')
const raySlab = require('ray-aabb-slab')
const Volume = require('./lib/volume')
const { BRICK_DIAMETER } = require('./lib/brick')
const collide = require('./lib/collision/aabb-obb')
const fill = require('ndarray-fill')

const cascadeDiameter = 16
const cascadeCount = 5
const cascadeTotalCells = Math.pow(cascadeDiameter, 2)
const cascades = []
for (var i = 0; i<cascadeCount; i++) {
  cascades.push(createCascade(i+1))
}

const min = Math.min
const max = Math.max
const floor = Math.floor
const ceil = Math.ceil

function hsl(p, a) {
  return `hsla(${p*360}, 100%, 65%, ${a||1})`
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

for (var x = -50; x<=50; x+=10) {
  for (var y = -50; y<=50; y+=10) {
    volumes[1].addBrick([x, y]).fill(_ => 1).empty=false
  }
}

const ctx = require('fc')(render, 1)
const camera = require('ctx-camera')(ctx, window, {})


function createCascade(level) {
  const ret = {
    grid: ndarray([], [cascadeDiameter, cascadeDiameter]),
    cellSize: Math.pow(2, level),
    prevCellSize: Math.pow(2, level - 1),
    percent: level/(cascadeCount + 1),
    color: hsl(level/(cascadeCount + 1))
  }

  ret.render = function(ctx) {
    var cellSize = Math.pow(2, level)
    var cur = cellSize * cascadeDiameter/2

    // ctx.beginPath()
    //   ctx.moveTo(-cur, -cur)
    //   ctx.lineTo(-cur, cur)
    //   ctx.lineTo(cur, cur)
    //   ctx.lineTo(cur, -cur)
    //   ctx.lineTo(-cur, -cur)
    //   ctx.fillStyle="#223"
    //   ctx.fill()

    ctx.beginPath()
      renderGrid(ctx, cellSize,
        -cur,
        -cur,
        cur,
        cur
      )
      ctx.strokeStyle = ret.color

      ctx.stroke()
  }

  ret.reset = function() {
    this.grid = ndarray([], [cascadeDiameter, cascadeDiameter])
  }

  ret.addBrick = function(cell, brick) {
    const o = this.grid.get(cell[0], cell[1])
    if (!o) {
      this.grid.get(cell[0], cell[1], [brick])
      return
    }
    o.push(brick)
  }
  return ret
}

const offsets = [
  [0, 1],
  [1, 1],
  [1, 0],
  [0, 0]
]
const out = vec2.create()

function brickAABB(brick, mat) {

  const aabb = {
    lower: [Number.MAX_VALUE, Number.MAX_VALUE],
    upper: [-Number.MAX_VALUE, -Number.MAX_VALUE]
  }

  const idx = [
    brick.index[0],
    brick.index[1]
  ]

  const txVerts = offsets.forEach((offset, i) => {
    vec2.transformMat3(
      out,
      vec2.add(out, idx, offset),
      mat
    )

    aabb.lower[0] = Math.min(aabb.lower[0], out[0])
    aabb.upper[0] = Math.max(aabb.upper[0], out[0])
    aabb.lower[1] = Math.min(aabb.lower[1], out[1])
    aabb.upper[1] = Math.max(aabb.upper[1], out[1])
  })
  return aabb
}

function render() {
  stock.rotation += 0.01
  //stock.pos[0] = -25
  //stock.pos[1] = -25

  volumes[0].pos = [Math.sin(Date.now() / 1000) * 200, Math.cos(Date.now() / 1000) * 25]
  stock.scale = [Math.abs(Math.sin(Date.now() / 1000) + 2) * 50, 1.0]
  ctx.clear()
  console.clear()
  camera.begin()
    center(ctx)
    ctx.scale(10, -10)
    ctx.lineCap = "round"
    ctx.lineWidth= .2

    // Render cascades from course to fine
    for (var i=cascadeCount-1; i>=0; i--) {
      // TODO: reset the cascades
      cascades[i].reset()
      cascades[i].render(ctx)

      // TODO: consider using the previous, coarser, cascade as a source of data
      volumes.forEach((volume) => {
        fillCascade(ctx, volume.modelMatrix, volume.bricks, cascades[i])
      })
    }

    volumes.forEach((volume) => volume.render(ctx))
  camera.end()
}

function fillCascade(ctx, mat, bricks, cascade) {
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
  bricks.forEach((brick) => {
    const txVerts = verts.map((vert, i) => {
      const out = vec2.create()
      vec2.transformMat3(
        out,
        vec2.add(out, brick.index, offsets[i]),
        mat
      )

      lower[0] = min(lower[0], out[0])
      upper[0] = max(upper[0], out[0])

      lower[1] = min(lower[1], out[1])
      upper[1] = max(upper[1], out[1])

      return out
    })

    // compute the index space bounding box
    start[0] = floor(lower[0]/cascade.cellSize)
    start[1] = floor(lower[1]/cascade.cellSize)
    end[0] = ceil(upper[0]/cascade.cellSize)
    end[1] = ceil(upper[1]/cascade.cellSize)
    // iterate over the bounding box
    for (var x = start[0]; x < end[0]; x += 1) {
      for (var y = start[1]; y < end[1]; y += 1) {
        cascadePos[0] = (x | 0) * cascade.cellSize
        cascadePos[1] = (y | 0) * cascade.cellSize

        const p = [
          [
            cascadePos[0],
            cascadePos[1]
          ],
          [
            cascadePos[0] + cascade.cellSize,
            cascadePos[1] + cascade.cellSize
          ]
        ];

        if (collide(p, txVerts)) {
          if (
            cascadePos[0] >= floor(cascade.prevCellSize * cascadeDiameter) ||
            cascadePos[1] >= floor(cascade.prevCellSize * cascadeDiameter) ||
            cascadePos[0] < -floor(cascade.prevCellSize * cascadeDiameter) ||
            cascadePos[1] < -floor(cascade.prevCellSize * cascadeDiameter)
          )

          {
            continue
          }

          ctx.fillStyle = hsl(cascade.percent, 1)
          ctx.fillRect(
            cascadePos[0] + cascade.cellSize * .35,
            cascadePos[1] + cascade.cellSize * .35,
            cascade.cellSize - cascade.cellSize * .7,
            cascade.cellSize - cascade.cellSize * .7
          )
        }
      }
    }
  })
}

