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
const moveable = createObject(mouse, 0.0, [50, 50], () => 0)
objects.push(moveable)
objects.push(createObject([0, 0], 0.0, [800, 20], () => 0))

const ctx = require('fc')(render, 1)
const center = require('ctx-translate-center')
const renderGrid = require('ctx-render-grid-lines')

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

function createObject(pos, rot, dims, sdfFn) {
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

    render(ctx, isect) {
      const hdx = this.dims[0] / 2
      const hdy = this.dims[1] / 2

      ctx.strokeStyle = isect ? "#aaa" : "red"
      ctx.beginPath()
        ctx.moveTo(this.points[0][0], this.points[0][1])
        ctx.lineTo(this.points[1][0], this.points[1][1])
        ctx.lineTo(this.points[2][0], this.points[2][1])
        ctx.lineTo(this.points[3][0], this.points[3][1])
        ctx.lineTo(this.points[0][0], this.points[0][1])
        ctx.stroke();

      // render normals
      ctx.strokeStyle = "red"
      this.normals.forEach(n => {
        ctx.beginPath()
          ctx.moveTo(this.pos[0], this.pos[1])
          ctx.lineTo(this.pos[0] + n[0] * 10, this.pos[1] + n[1] * 10)
          ctx.stroke()
      })

      ctx.fillStyle = "white"
      ctx.beginPath()
      this.points.forEach(p => {
        ctx.moveTo(p[0], p[1])
        ctx.arc(p[0], p[1], 2, 0, Math.PI*2, false)
        ctx.fill()
      })
    }
  }

  out.tick(true)

  return out;
}



function render() {
  const now = Date.now() /10
  vec2.set(eye,
    Math.sin(now / 1000) * 200,
    Math.cos(now / 1000) * 200
  )
  mat3.identity(model)
  var scale =  [1 + Math.abs(Math.sin(now / 100)), 1 + Math.abs(Math.cos(now / 1000))]

  // render prereqs
  mat3.invert(invModel, model)
  var invEye = txo([0, 0], invModel, eye)
  var txOrigin = txo([0, 0], model, origin)

    ctx.clear()
    camera.begin()
      center(ctx)
      ctx.scale(2.0, -2.0)
      ctx.pointToWorld(moveable.pos, camera.mouse.pos)
      moveable.rot += 0.01
      objects[1].rot += 0.01

      objects.forEach(o => o.tick())

      const isect = !SAT(
        objects[1].points,
        objects[0].points
      )

      objects.forEach((o) => {
        o.render(ctx, isect)
      })
      //trace()
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
