const ndarray = require('ndarray')
const fill = require('ndarray-fill')
const glmatrix = require('gl-matrix')
const vec2 = glmatrix.vec2
const mat3 = glmatrix.mat3
const SAT = require('./lib/collision/obb-obb')
const v2scratch = vec2.create()
const objectNormals = [
  [-1, 0],
  [ 0, -1],
  [ 1,  0],
  [ 0,  1]
]
const objects = []
const mouse = [0, 100];
const CELL_RADIUS = 10

function createOccupancyGrid(dims) {
  // scratch variable
  const unpacked = {
    time: 0,
    object: 0
  }

  function unpackCell(v) {
    unpacked.time = (v >> 24) & 0xFF
    unpacked.object = v & 0x1000000
    return unpacked
  }

  function pack(time, id) {
    return ((time && 0xFF) << 24) | (id & 0x1000000)
  }

  const ret = {
    cellRadius: CELL_RADIUS,
    dims: dims,
    // distance in time when evaluating whether a cell is still occupied
    timeDelta: 1,
    time: 1,
    data: ndarray([], dims),
    tick() {
      // TODO: if we do CCD this will need to adapt to the the fastest
      // moving object or something....
      this.time++
    },

    // returns objectId if already occupied
    set(v2, objectId) {
      const x = (v2[0] / this.cellRadius)|0
      const y = (v2[1] / this.cellRadius)|0

      var v = this.data.get(x, y) || {}
      v.time = this.time
      v.object = objectId

      this.data.set(x, y, v)
    },

    occupied(v2, objectId) {
      const x = (v2[0] / this.cellRadius)|0
      const y = (v2[1] / this.cellRadius)|0
      const v = this.data.get(x, y)

      if (!v) {
        return false
      }

      if (v.object === objectId) {
        return false
      }

      if (this.time - v.time >= this.timeDelta) {
        return false
      }

      return v.object
    },

    render(ctx, objects) {
      const l = objects.length

      ctx.strokeStyle = "#444"
      for (var x=0; x<dims[0]; x++) {
        for (var y=0; y<dims[1]; y++) {
          ctx.strokeRect(
            x * this.cellRadius + 2,
            y * this.cellRadius + 2,
            this.cellRadius - 4,
            this.cellRadius - 4
          )

          var v = this.data.get(x, y)

          if (v) {
            var dt = this.time - v.time

            if (dt < this.timeDelta) {
              ctx.fillStyle = hsl(1.0 - Math.min(1.0, 1.0 - dt/this.timeDelta) * 0.7)
              ctx.fillRect(
                x * this.cellRadius + 3,
                y * this.cellRadius + 3,
                this.cellRadius - 6,
                this.cellRadius - 6
              )
            }
          }

        }
      }
    }
  }

  return ret
}

function grey(p) {
  return `rgba(255, 255, 255, ${p})`
}

const grid = createOccupancyGrid([100, 100])


const moveable = createObject([0, 0], 0.0, [90, 50], CELL_RADIUS)
moveable.rot = Math.PI/4
//objects.push(createObject([0, 0], 0.0, [800, 20], CELL_RADIUS))
// objects[objects.length-1].rot = 0.4
objects.push(moveable)
//objects.push(createObject([300, 90], 0.0, [200, 70], CELL_RADIUS))
// objects[objects.length-1].rot = 0.4
// for (var i=0; i<100; i++) {
//   addRandomObject()
// }

function addRandomObject() {
  var o = createObject(
    [(0.001 + Math.random()) * 1000, 900],
    0.0,
    [10 + (0.001 + Math.random()) * 40, 20 + Math.random() * 40],
    CELL_RADIUS
  )
  o.rot = Math.random() * Math.PI*2
  objects.push(o)
}

const ctx = require('fc')(render, 1)
const center = require('ctx-translate-center')
const renderGrid = require('ctx-render-grid-lines')

const segseg = require('segseg')
const raySlab = require('ray-aabb-slab')
const camera = require('ctx-camera')(ctx, window, {})

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
const v2tmp = vec2.create()
const v2tmp2 = vec2.create()
const v2dir = vec2.create()
const v2offset = vec2.create()
const v2gridPos = vec2.create()


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

function normalRotate(n, i, r) {
  const c = Math.cos(r)
  const s = Math.sin(r)
  const x = objectNormals[i][0]
  const y = objectNormals[i][1]
  const nx = x * c - y * s
  const ny = x * s + y * c
  n[0] = nx
  n[1] = ny
}

function createObject(pos, rot, dims, cellRadius) {
  const gridDims = [(dims[0]/cellRadius)|0, (dims[1]/cellRadius)|0]

  const out = {
    rot: rot,
    pos: [pos[0], pos[1]],
    velocity: [0, 0],
    angularVelocity: 0,
    mass: 1,
    invMas: 1,
    dims: dims,
    model: mat3.create(),
    invModel: mat3.create(),
    /*
      normal/face layout
              3
              |
           o----o
      0 - |     | - 2
          o----o
            |
            1
    */
    normals: [
      vec2.create(),
      vec2.create(),
      vec2.create(),
      vec2.create()
    ],

    points: [
      vec2.create(),
      vec2.create(),
      vec2.create(),
      vec2.create()
    ],

    grid: ndarray(new Float32Array(gridDims[0] * gridDims[1]), gridDims),

    aabb() {
      var lx = Infinity
      var ly = Infinity
      var ux = -Infinity
      var uy = -Infinity

      this.points.forEach((p) => {
        lx = Math.min(lx, p[0])
        ly = Math.min(ly, p[1])
        ux = Math.max(ux, p[0])
        uy = Math.max(uy, p[1])
      })

      return [
        [lx, ly],
        [ux, uy]
      ]
    },
    dirty: true,
    tick(force, objectId, occupancyGrid) {
      const hdx = this.dims[0] / 2
      const hdy = this.dims[1] / 2
      const p = [0, 0]
      const gp = [0, 0]
      const ogp = [0, 0]

      this.dirty = this.dirty || this.angularVelocity
      this.dirty = this.dirty || (this.velocity[0] || this.velocity[1])

      if (!this.dirty) {
        for (var x = 0; x<this.dims[0]; x++) {
          for (var y = 0; y<this.dims[1]; y++) {
            p[0] = (x - hdx + 0.5)
            p[1] = (y - hdy + 0.5)

            vec2.transformMat3(gp, p, this.model)

            occupancyGrid.set(gp, objectId)
          }
        }

        return
      }

      // this.velocity[1] -= 0.1
      //this.angularVelocity *= 0.

      this.rot += this.angularVelocity;
      var newX = this.pos[0] + this.velocity[0];
      var newY = Math.max(this.pos[1] + this.velocity[1], this.dims[1]);
      var newMat = mat3.create()

      var dx = newX - this.pos[0]
      var dy = newY - this.pos[1]

      mat3.translate(newMat, newMat, [newX, newY])
      mat3.rotate(newMat, newMat, this.rot)

      // write all of the objects grid cells into the occupancy grid!

      var blocked = false
      const impulse = [0, 0]
      var angular = 0
      const contacts = {}
      var hasContacts = false
      for (var x = 0; x<this.dims[0]; x++) {
        for (var y = 0; y<this.dims[1]; y++) {
          p[0] = (x - hdx + 0.5)// / CELL_RADIUS//
          p[1] = (y - hdy + 0.5)// / CELL_RADIUS//

          vec2.transformMat3(gp, p, newMat)
          vec2.transformMat3(ogp, p, this.model)

          var otherId = occupancyGrid.occupied(gp, objectId)
          if (otherId === false) {

            continue
          }
          hasContacts = true

          if (!contacts[otherId]) {
            contacts[otherId] = []
          }

          contacts[otherId].push({
            last: [ogp[0], ogp[1]],
            next: [gp[0], gp[1]],
          })

          if (otherId !== false) {
            var side = Math.sign(gp[0] - this.pos[0])
            var d = vec2.distance(this.pos, gp)
            angular = (angular + side * d * 0.001) / 2.0

            // other.pos[0] += (other.pos[0] - this.pos[0])
            // other.pos[1] += (other.pos[1] - this.pos[1])
          }
        }
      }

      if (hasContacts) {
        // if we are blocked in the new position then don't apply the current
        // update

        if (this.processContacts(contacts) === false) {
          return
        }
      }

      // now we figure out all of the impulses


      // this.pos[0] = dx
      // this.pos[1] = dy
      this.model = newMat


      mat3.invert(this.invModel, this.model)
      txo(v2scratch, this.model, this.pos)

      // recompute normals for collision detection
      this.normals.forEach((n, i) => normalRotate(n, i, this.rot))

      // recompute the points for collision detection

      vec2.set(v2scratch, -hdx, -hdy)
      txo(this.points[0], this.model, v2scratch)

      vec2.set(v2scratch, hdx, -hdy)
      txo(this.points[1], this.model, v2scratch)

      vec2.set(v2scratch, hdx, hdy)
      txo(this.points[2], this.model, v2scratch)

      vec2.set(v2scratch, -hdx, hdy)
      txo(this.points[3], this.model, v2scratch)

      for (var x = 0; x<this.dims[0]; x++) {
        for (var y = 0; y<this.dims[1]; y++) {
          p[0] = (x - hdx + 0.5)
          p[1] = (y - hdy + 0.5)

          vec2.transformMat3(gp, p, this.model)

          occupancyGrid.set(gp, objectId)
        }
      }

      this.dirty = false
    },

    processContacts(contactGroups) {
      console.log("ASDFASDF")
      const ids = Object.keys(contactGroups)

      ids.forEach(id => {
        const obj = objects[id]
        var x = obj.pos[0]
        var y = obj.pos[1]
        contactGroups[id].forEach(contact => {
          const vx = contact.last[0] - contact.next[0]
          const vy = contact.last[1] - contact.next[1]

          this.velocity[0] = (this.velocity[0] + vx) / 2.0
          this.velocity[1] = (this.velocity[1] + vy) / 2.0

          // obj.velocity[0] += vx // (obj.velocity[0] - vx * 1000) / 2.0
          // obj.velocity[1] += vy // (obj.velocity[1] - vy * 1000) / 2.0
          obj.dirty = true
          x -= vx // * 100.1
          y -= vy // * 100.1
        })

        console.log(obj.pos[0] - x, obj.pos[1] - y)
      })
      return false
    },

    render(ctx, isect, regionAABB) {
      const hdx = this.dims[0] / 2
      const hdy = this.dims[1] / 2

      // draw grid
      ctx.strokeStyle = "#666"
      const sx = this.points[0][0]
      const sy = this.points[0][1]

      var oxx = this.normals[2][0] * cellRadius
      var oxy = this.normals[2][1] * cellRadius
      var oyx = this.normals[3][0] * cellRadius
      var oyy = this.normals[3][1] * cellRadius

      ctx.strokeStyle = "aaa" //isect ? "red" : "#aaa"
      ctx.beginPath()
        ctx.moveTo(this.points[0][0], this.points[0][1])
        ctx.lineTo(this.points[1][0], this.points[1][1])
        ctx.lineTo(this.points[2][0], this.points[2][1])
        ctx.lineTo(this.points[3][0], this.points[3][1])
        ctx.lineTo(this.points[0][0], this.points[0][1])
        ctx.stroke();

      // render normals
      // ctx.strokeStyle = "red"
      // this.normals.forEach(n => {
      //   ctx.beginPath()
      //     ctx.moveTo(this.pos[0], this.pos[1])
      //     ctx.lineTo(this.pos[0] + n[0] * 10, this.pos[1] + n[1] * 10)
      //     ctx.stroke()
      // })

      ctx.fillStyle = "white"
      ctx.beginPath()
      this.points.forEach(p => {
        ctx.moveTo(p[0], p[1])
        ctx.arc(p[0], p[1], 1, 0, Math.PI*2, false)
        ctx.fill()
      })
    }
  }

  return out;
}

setInterval(() =>{
  if (keys.KeyA) {
    addRandomObject()
  }
}, 40)

const keys = {}
window.addEventListener('keydown', (e) => keys[e.code] = true)
window.addEventListener('keyup', (e) => keys[e.code] = false)

function render() {
  ctx.clear()
  camera.begin()
    // center(ctx)
    ctx.scale(2.0, -2.0)
    ctx.translate(0, -window.innerHeight)
    ctx.pointToWorld(moveable.pos, camera.mouse.pos)
    moveable.velocity[0] = 0
    moveable.velocity[1] = 0
    moveable.rot += 0.01
    moveable.dirty = true

    grid.tick()

    objects.forEach((obj, i) => {
      obj.tick(false, i, grid)
      obj.render(ctx)
    })

    grid.render(ctx, objects)
  camera.end();
}

function hsl(p, a) {
  return `hsla(${p*360}, 100%, 46%, ${a||1})`
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
