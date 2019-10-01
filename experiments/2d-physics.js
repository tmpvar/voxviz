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
const moveable = createObject([0, 0], 0.0, [50, 50], CELL_RADIUS)
moveable.rot = Math.PI/4
objects.push(moveable)
objects.push(createObject([0, 0], 0.0, [800, 20], CELL_RADIUS))
objects.push(createObject([-20, 90], 0.0, [80, 80], CELL_RADIUS))

const ctx = require('fc')(render, 1)
const center = require('ctx-translate-center')
const renderGrid = require('ctx-render-grid-lines')

const segseg = require('segseg')
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

function aabbIntersection(a, b) {
  const l = [
    Math.max(a[0][0], b[0][0]),
    Math.max(a[0][1], b[0][1])
  ]
  const u = [
    Math.min(a[1][0], b[1][0]),
    Math.min(a[1][1], b[1][1])
  ]

  return [l, u]
}

function createObject(pos, rot, dims, cellRadius) {
  const gridDims = [(dims[0]/cellRadius)|0, (dims[1]/cellRadius)|0]
  const out = {
    rot: rot,
    pos: [pos[0], pos[1]],
    dims: dims,
    model: mat3.create(),
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

    tick(force) {
      var dirty = false
      dirty = dirty || rot !== this.rot
      dirty = dirty || (pos[0] !== this.pos[0] || pos[1] !== this.pos[1])

      if (!force && !dirty) {
        return
      }

      mat3.identity(this.model)

      mat3.translate(this.model, this.model, this.pos)
      mat3.rotate(this.model, this.model, this.rot)

      rot = this.rot
      pos[0] = this.pos[0]
      pos[1] = this.pos[1]

      txo(v2scratch, this.model, this.pos)

      // recompute normals for collision detection
      this.normals.forEach((n, i) => normalRotate(n, i, this.rot))

      // recompute the points for collision detection
      const hdx = this.dims[0] / 2
      const hdy = this.dims[1] / 2
      vec2.set(v2scratch, -hdx, -hdy)
      txo(this.points[0], this.model, v2scratch)

      vec2.set(v2scratch, hdx, -hdy)
      txo(this.points[1], this.model, v2scratch)

      vec2.set(v2scratch, hdx, hdy)
      txo(this.points[2], this.model, v2scratch)

      vec2.set(v2scratch, -hdx, hdy)
      txo(this.points[3], this.model, v2scratch)
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

      // for (var x = 0; x < gridDims[0]; x++) {
      //   for (var y = 0; y < gridDims[1]; y++) {
      //     ctx.beginPath()
      //       ctx.moveTo(
      //         sx + x * oxx,
      //         sy + x * oxy
      //       )
      //       ctx.lineTo(
      //         sx + x * oxx + oxx,
      //         sy + x * oxy + oxy
      //       )
      //       ctx.lineTo(
      //         sx + x * oxx + oxx + y * oyx + oyx,
      //         sy + x * oxy + oxy + y * oyy + oyy
      //       )
      //       ctx.lineTo(
      //         sx + x * oxx + y * oyx + oyx,
      //         sy + x * oxy + y * oyy + oyy
      //       )
      //       ctx.lineTo(
      //         sx + x * oxx,
      //         sy + x * oxy
      //       )
      //       ctx.stroke();
      //   }
      // }

      // ctx.strokeStyle = isect ? "red" : "#aaa"
      ctx.strokeStyle = "#aaa"
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
    },

    renderIsect(ctx, regionAABB) {

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

      const aabb = [
        regionAABB[0],
        [regionAABB[1][0], regionAABB[0][1]],
        regionAABB[1],
        [regionAABB[0][0], regionAABB[1][1]]
      ]
      ctx.strokeStyle = "#f0f"
      ctx.beginPath()
        ctx.moveTo(aabb[0][0], aabb[0][1])
        ctx.lineTo(aabb[1][0], aabb[1][1])
        ctx.lineTo(aabb[2][0], aabb[2][1])
        ctx.lineTo(aabb[3][0], aabb[3][1])
        ctx.lineTo(aabb[0][0], aabb[0][1])
        ctx.stroke();

      ctx.strokeStyle = "orange"
      var i = 0
      for (var x = 0; x < gridDims[0]; x++) {
        for (var y = 0; y < gridDims[1]; y++) {
          i++
          // if (i>3) return
          ctx.save()
            ctx.lineWidth = 1.0

            var cx = x * cellRadius
            var cy = y * cellRadius
            // var cx = sx + x * oxx + y * oyx
            // var cy = sy + x * oxy + y * oyy

            // var b = [
            //   [cx, cy],
            //   [cx + oxx, cy + oxy],
            //   [cx + oxx + oyx, cy + oxy + oyy],
            //   [cx, cy + oxy + oyy],
            //   // [sx + x * oxx + oxx + y * oyx + oyx, sy + y * oxy + oxy + y * oyy + oyy],
            //   // [sx + x * oxx + y * oyx + oyx, sy + y * oxy + y * oyy + oyy]
            // ]

            var b = [
              txo([0, 0], this.model, [cx - hdx, cy - hdy]),
              txo([0, 0], this.model, [(cx + cellRadius) - hdx, cy - hdy]),
              txo([0, 0], this.model, [(cx + cellRadius) - hdx, (cy + cellRadius) - hdy]),
              txo([0, 0], this.model, [cx - hdx, (cy + cellRadius) - hdy]),
            ]

            var isect = !SAT(b, aabb)
            if (isect) {
              ctx.restore();
              continue
            }

            ctx.beginPath()
              ctx.moveTo(b[0][0], b[0][1])
              ctx.lineTo(b[1][0], b[1][1])
              ctx.lineTo(b[2][0], b[2][1])
              ctx.lineTo(b[3][0], b[3][1])
              ctx.lineTo(b[0][0], b[0][1])
              ctx.stroke();
          ctx.restore()

        }
      }
    }
  }

  out.tick(true)

  return out;
}

function render() {
  ctx.clear()
  camera.begin()
    center(ctx)
    ctx.scale(2.0, -2.0)
    ctx.pointToWorld(moveable.pos, camera.mouse.pos)
    moveable.rot += 0.01

    objects.forEach(o => o.tick())

    const isect = SAT(
      objects[1].points,
      objects[0].points
    )

    objects.forEach((o, i) => {
      o.render(ctx, false)

    })

    if (isect) {
      const regionAABB = [[Infinity, Infinity], [-Infinity, -Infinity]]
      const aabb = aabbIntersection(objects[0].aabb(), objects[1].aabb())
      const aabbCenter = [aabb[1][0] - aabb[0][0], aabb[1][1] - aabb[0][1]]

      ctx.strokeStyle = "purple"
      const lx = Math.floor(aabb[0][0] / CELL_RADIUS) * CELL_RADIUS
      const ly = Math.floor(aabb[0][1] / CELL_RADIUS) * CELL_RADIUS
      const ux = Math.ceil(aabb[1][0] / CELL_RADIUS) * CELL_RADIUS
      const uy = Math.ceil(aabb[1][1] / CELL_RADIUS) * CELL_RADIUS

      const aabbPoints = [
        [lx, ly],
        [ux, ly],
        [ux, uy],
        [lx, uy]
      ]

      ctx.strokeRect(
        lx,
        ly,
        ux - lx,
        uy - ly
      )

      ctx.strokeStyle = "white"
      for (var x=lx; x<ux; x+=CELL_RADIUS) {
        for (var y=ly; y<uy; y+=CELL_RADIUS) {

          objects.forEach((o, i) => {

            // TODO: this check should only be done on pairs so for now we
            // reduce the problem set by only dealing with object[0]
            if (i > 0) return

            const region = []


            o.points.forEach((cp, pi) => {
              const cn = o.points[(pi+1) % o.points.length]

              if (cp[0] >= lx && cp[0] <= ux &&
                  cp[1] >= ly && cp[1] <= uy)
              {
                region.push([cp[0], cp[1]])
              }

              aabbPoints.forEach((ap, ai) => {
                const an = aabbPoints[(ai+1) % aabbPoints.length]

                const r = segseg(
                  cp[0], cp[1], cn[0], cn[1],
                  ap[0], ap[1], an[0], an[1]
                )

                if (r && r !== true) {
                  region.push(r)
                }
              })
            })

            region.forEach(r => {
              regionAABB[0][0] = Math.min(regionAABB[0][0], r[0])
              regionAABB[0][1] = Math.min(regionAABB[0][1], r[1])
              regionAABB[1][0] = Math.max(regionAABB[1][0], r[0])
              regionAABB[1][1] = Math.max(regionAABB[1][1], r[1])
            })

            // inflate the regionAABB to cover full grid cells (world space)
            regionAABB[0][0] = Math.floor(regionAABB[0][0] / CELL_RADIUS) * CELL_RADIUS
            regionAABB[0][1] = Math.floor(regionAABB[0][1] / CELL_RADIUS) * CELL_RADIUS
            regionAABB[1][0] = Math.ceil(regionAABB[1][0] / CELL_RADIUS) * CELL_RADIUS
            regionAABB[1][1] = Math.ceil(regionAABB[1][1] / CELL_RADIUS) * CELL_RADIUS

            // // render the region
            // ctx.beginPath()
            //   ctx.strokeStyle = "#f0f"
            //   ctx.moveTo(region[0][0], region[0][1])
            //   for (var i=1; i<region.length; i++) {
            //     ctx.lineTo(region[i][0], region[i][1])
            //   }
            //   ctx.lineTo(region[0][0], region[0][1])
            //   ctx.stroke()


            const sx = o.points[0][0]
            const sy = o.points[0][1]

            var oxx = o.normals[2][0] * CELL_RADIUS
            var oxy = o.normals[2][1] * CELL_RADIUS
            var oyx = o.normals[3][0] * CELL_RADIUS
            var oyy = o.normals[3][1] * CELL_RADIUS

            ctx.strokeStyle = "green"
            ctx.strokeRect(
              regionAABB[0][0],
              regionAABB[0][1],
              regionAABB[1][0] - regionAABB[0][0],
              regionAABB[1][1] - regionAABB[0][1]
            )

          })

          // ctx.strokeRect(
          //   x + 4,
          //   y + 4,
          //   CELL_RADIUS - 8,
          //   CELL_RADIUS - 8
          // )

        }
      }

      // objects.forEach((o) => {
      //   o.renderIsect(ctx, regionAABB)
      // })
      objects[0].renderIsect(ctx, regionAABB)
    }

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
