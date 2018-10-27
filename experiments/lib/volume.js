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
    const invVolumeModel = mat3.invert(mat3.create(), this.modelMatrix)

    // tool index space -> stock index space
    const toolToVolume = mat3.create()
    mat3.multiply(toolToVolume, invVolumeModel, toolModel)

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
      ctx.lineWidth = 0.025
      ctx.beginPath()

      const txVerts = verts.map((vert, i) => {
        const out = vec2.create()
        vec2.transformMat3(
          out,
          vec2.add(out, toolBrick.index, offsets[i]),
          toolToVolume
        )

        i === 0
        ? ctx.moveTo(out[0], out[1])
        : ctx.lineTo(out[0], out[1])

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
      ctx.lineWidth = 0.01
      ctx.strokeRect(start[0], start[1], end[0] - start[0], end[1] - start[1])

      // iterate over the bounding box
       for (var x = start[0]; x < end[0]; x+=1) {
        for (var y = start[1]; y < end[1]; y+=1) {
          brickIndex[0] = x|0
          brickIndex[1] = y|0



          var pos = [x, y]

          const p = [brickIndex, [brickIndex[0]+1, brickIndex[1]+1]]
          if (collide(p, txVerts)) {
            // TODO: this can be optimized by running SAT over the
            //       aabb (stock brick) and the obb (tool brick)
            //this.getBrick(brickIndex, true)
            var stockBrick = this.getBrick(brickIndex, true)
            opAddBrick(toolToVolume, toolBrick, stockBrick, pos)
          }
        }
      }

    })
  }
}

function edgeTest(start, end, px, py) {
  return ((px - start[0]) * (end[1] - start[1]) - (py - start[1]) * (end[0] - start[0]) >= 0)
}

function opAddBrick(tx, toolBrick, stockBrick, brickStart) {
  // var px = x
  // var py = y
  // if (
  //   edgeTest(txVerts[0], txVerts[1], px, py) &&
  //   edgeTest(txVerts[1], txVerts[2], px, py) &&
  //   edgeTest(txVerts[2], txVerts[3], px, py) &&
  //   edgeTest(txVerts[3], txVerts[0], px, py)
  // ) {
  //   var stockBrick = this.getBrick(brickIndex, true)
  //   opAddBrick(toolToVolume, toolBrick, stockBrick, pos)
  // }
  //

  const INV_BRICK_DIAMETER = 1/BRICK_DIAMETER
  const offset = INV_BRICK_DIAMETER / 2.0
  const lower = [0, 0]
  const upper = [0, 0]
  const start = [0, 0]
  const end = [0, 0]
  const pos = [0, 0]
  for (var x = 0; x<1; x+=INV_BRICK_DIAMETER) {
    for (var y = 0; y<1; y+=INV_BRICK_DIAMETER) {
      lower[0] = x
      lower[1] = y
      upper[0] = x + offset
      upper[1] = y + offset

      // transform each tool voxel into stock voxel space
      vec2.transformMat3(lower, lower, tx)
      vec2.transformMat3(upper, upper, tx)

      start[0] = (min(lower[0], upper[0]) - brickStart[0]) * BRICK_DIAMETER
      start[1] = (min(lower[1], upper[1]) - brickStart[1]) * BRICK_DIAMETER
      end[0]   = (max(lower[0], upper[0]) - brickStart[0]) * BRICK_DIAMETER
      end[1]   = (max(lower[1], upper[1]) - brickStart[1]) * BRICK_DIAMETER


      var t = toolBrick.grid.get((x * BRICK_DIAMETER)|0, (y * BRICK_DIAMETER)|0)
      if (!t) {
        continue
      }

      //console.log(start, end, lower, upper, brickStart)

      for (var a=start[0]; a<end[0]; a+=1) {
        for (var b=start[1]; b<end[1]; b+=1) {
          if (a < 0 || b < 0 || a >= BRICK_DIAMETER || b >= BRICK_DIAMETER) {
           continue
          }

          stockBrick.grid.set(a|0, b|0, t)
          stockBrick.empty = false
        }
      }
    }
  }

}


module.exports = Volume
