const { mat3, vec2 } = require('gl-matrix')
const { Brick, BRICK_DIAMETER } = require('./brick')
const square = require('./square')
const tx = require('./tx')
const fill = require('ndarray-fill')
const collide = require('./collision/aabb-obb')
const min = Math.min
const max = Math.max
const floor = Math.floor
const ceil = Math.ceil
const round = Math.round


const one = [1,1]

function hashKey(v) {
  return v[0] + ',' + v[1]
}

class Volume {
  constructor(pos) {
    this.pos = pos
    this.scale = [1, 1]
    this.rotation = 0
    this.bricks = new Map()
  }

  get aabb() {
    const aabb = [
      Number.MAX_VALUE,
      Number.MAX_VALUE,
      -Number.MAX_VALUE,
      -Number.MAX_VALUE
    ]
    // TODO: only do this when dirty
    this.bricks.forEach((brick) => {
      aabb[0] = Math.min(brick.index[0], aabb[0])
      aabb[1] = Math.min(brick.index[1], aabb[1])
      aabb[2] = Math.max(brick.index[0] + 1, aabb[2])
      aabb[3] = Math.max(brick.index[1] + 1, aabb[3])
    })

    return aabb
  }

  get modelMatrix() {
    const model = mat3.create()
    mat3.translate(model, model, this.pos)
    mat3.rotate(model, model, this.rotation)
    mat3.scale(model, model, this.scale)

    return model
  }

  getBrickModelMatrix(brick) {
    const model = mat3.create()
    mat3.translate(model, model, vec2.add(vec2.create(), this.pos, brick.index))
    mat3.rotate(model, model, this.rotation)
    mat3.scale(model, model, this.scale)
    return model
  }

  addBrick(index) {
    const brick = new Brick(index)
    this.bricks.set(hashKey(index), brick)
    return brick
  }

  getBrick(index, create) {
    const key = hashKey(index)
    const brick = this.bricks.get(key)
    if (brick) {
      return brick
    }
    if (!create) {
      return
    }

    return this.addBrick(index)
  }

  render(ctx) {
    const aabb = this.aabb
    const model = this.modelMatrix

    this.bricks.forEach((brick) => {
      ctx.lineWidth = 0.015
      ctx.beginPath()
      square(
        ctx,
        model,
        brick.index[0],
        brick.index[1],
        1,
        1
      )
      ctx.strokeStyle = "#FFF"
      ctx.stroke()

      !brick.empty && brick.render(ctx, model)
    })
  }

  opAdd (ctx, tool) {
    const toolModel = tool.modelMatrix
    const stockModel = this.modelMatrix
    const invStockModel = mat3.invert(mat3.create(), stockModel)
    const invToolModel = mat3.invert(mat3.create(), toolModel)

    // tool index space -> stock index space
    const toolToStock = mat3.create()
    mat3.multiply(toolToStock, invStockModel, toolModel)
    // stock index space -> tool index space
    const stockToTool = mat3.create()
    mat3.multiply(stockToTool, invToolModel, stockModel)

    const lower = [Number.MAX_VALUE, Number.MAX_VALUE]
    const upper = [-Number.MAX_VALUE, -Number.MAX_VALUE]
    const start = vec2.create()
    const end = vec2.create()

    const verts = [
      vec2.create(),
      vec2.create(),
      vec2.create(),
      vec2.create()
    ]

    const offsets = [
      [0, 1],
      [1, 1],
      [1, 0],
      [0, 0]
    ]

    const brickIndex = [0, 0]

    tool.bricks.forEach((toolBrick) => {
      const txVerts = verts.map((vert, i) => {
        const out = vec2.create()
        vec2.transformMat3(
          out,
          vec2.add(out, toolBrick.index, offsets[i]),
          toolToStock
        )

        lower[0] = min(lower[0], out[0])
        upper[0] = max(upper[0], out[0])

        lower[1] = min(lower[1], out[1])
        upper[1] = max(upper[1], out[1])

        return out
      })
      ctx.closePath()
      ctx.strokeStyle = 'yellow'
      ctx.stroke()

      // compute the stock index space bounding box
      start[0] = floor(lower[0])
      start[1] = floor(lower[1])
      end[0] = ceil(upper[0])
      end[1] = ceil(upper[1])

      // iterate over the bounding box
      for (var x = start[0]; x < end[0]; x += 1) {
        for (var y = start[1]; y < end[1]; y += 1) {
          brickIndex[0] = floor(x)
          brickIndex[1] = floor(y)

          const p = [brickIndex, [brickIndex[0] + 1, brickIndex[1] + 1]]
          if (collide(p, txVerts)) {
            // TODO: this can be optimized by running SAT over the
            //       aabb (stock brick) and the obb (tool brick)
            var stockBrick = this.getBrick(brickIndex, true)
            opAddBrick(stockToTool, toolBrick, stockBrick, txVerts)
          }
        }
      }
    })
  }
}

function edgeTest (start, end, px, py) {
  return ((px - start[0]) * (end[1] - start[1]) - (py - start[1]) * (end[0] - start[0]) >= 0)
}

function opAddBrick (stockToTool, toolBrick, stockBrick, toolVerts) {
  const INV_BRICK_DIAMETER = 1 / BRICK_DIAMETER
  const offset = 0.5
  const v2 = vec2.create()

  for (var x = 0; x < BRICK_DIAMETER; x += 1) {
    for (var y = 0; y < BRICK_DIAMETER; y += 1) {
      // skip voxels that are already filled
      if (stockBrick.grid.get(x, y)) {
        // TODO: this is an optimization to avoid further computation
        //       when the stock voxel is already set. For debugging purposes
        //       it has been commented out.
        // continue
      }

      v2[0] = stockBrick.index[0] + (x + offset) * INV_BRICK_DIAMETER
      v2[1] = stockBrick.index[1] + (y + offset) * INV_BRICK_DIAMETER

      // is this position inside of the transformed tool?
      if (
        edgeTest(toolVerts[0], toolVerts[1], v2[0], v2[1]) &&
        edgeTest(toolVerts[1], toolVerts[2], v2[0], v2[1]) &&
        edgeTest(toolVerts[2], toolVerts[3], v2[0], v2[1]) &&
        edgeTest(toolVerts[3], toolVerts[0], v2[0], v2[1])
      ) {
        // transform the point back into tool space
        vec2.transformMat3(v2, v2, stockToTool)
        var toolVoxelX = ((v2[0] - toolBrick.index[0]) * BRICK_DIAMETER) | 0
        var toolVoxelY = ((v2[1] - toolBrick.index[1]) * BRICK_DIAMETER) | 0
        var toolValue = toolBrick.grid.get(toolVoxelX, toolVoxelY)
        if (toolValue) {
          stockBrick.grid.set(x, y, toolValue)
          stockBrick.empty = false
        }
      }
    }
  }
}

module.exports = Volume
