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
const moveable = createObject([0, 0], 0.0, [90, 50], CELL_RADIUS)
// moveable.rot = Math.PI/4
objects.push(createObject([0, 0], 0.0, [800, 20], CELL_RADIUS))
objects[objects.length-1].rot = 0.4
objects.push(moveable)
objects.push(createObject([300, 90], 0.0, [200, 70], CELL_RADIUS))


objects[objects.length-1].rot = 0.4
// for (var i=0; i<100; i++) {
//   var o = createObject([Math.random() * 1000 - 250, Math.random() * 400 + 20], 0.0, [10 + Math.random() * 40, 20 + Math.random() * 40], CELL_RADIUS)
//   o.rot = Math.random() * Math.PI*2
//   objects.push(o)
// }

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
  console.log(pos)
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

    tick(force) {
      var dirty = false
      //
      // this.velocity[1] -= 0.01
      //
      // this.rot += this.angularVelocity;
      // this.pos[0] += this.velocity[0];
      // this.pos[1] += this.velocity[1];
      //
      // // TODO: don't limit on y
      // this.pos[1] = Math.max(this.pos[1], 0)

      dirty = dirty || rot !== this.rot
      dirty = dirty || (pos[0] !== this.pos[0] || pos[1] !== this.pos[1])

      if (!force && !dirty) {
        return
      }

      mat3.identity(this.model)
      mat3.translate(this.model, this.model, this.pos)
      mat3.rotate(this.model, this.model, this.rot)

      mat3.invert(this.invModel, this.model)

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
    },

    renderIsect(ctx, regionAABB, other) {
      // const hdx = this.dims[0] / 2
      // const hdy = this.dims[1] / 2
      // const bhdx = other.dims[0] / 2
      // const bhdy = other.dims[1] / 2

      // draw grid
      ctx.strokeStyle = "#666"
      const sx = this.points[0][0]
      const sy = this.points[0][1]

      // TODO: maybe this would be faster that transforming every grid cell
      // var oxx = this.normals[2][0] * cellRadius
      // var oxy = this.normals[2][1] * cellRadius
      // var oyx = this.normals[3][0] * cellRadius
      // var oyy = this.normals[3][1] * cellRadius

      /* Process:
       - walk over the region aabb
       - for a and b
         - convert region cell into local grid space
         - if grid cell is occupied return both true and world space grid center
        - if both a and b are occupied
          - compute impulse between centers
        - average impulse and apply to a's velocity
      */


      // const aabb = [
      //   regionAABB[0],
      //   [regionAABB[1][0], regionAABB[0][1]],
      //   regionAABB[1],
      //   [regionAABB[0][0], regionAABB[1][1]]
      // ]
      const points = other.points
      ctx.strokeStyle = "#f0f"
      ctx.beginPath()
        ctx.moveTo(points[0][0], points[0][1])
        ctx.lineTo(points[1][0], points[1][1])
        ctx.lineTo(points[2][0], points[2][1])
        ctx.lineTo(points[3][0], points[3][1])
        ctx.lineTo(points[0][0], points[0][1])
        ctx.stroke();

      ctx.strokeStyle = "orange"
      var impulse = [0, 0]
      var aworld = [0, 0]
      var bworld = [0, 0]
      var halfCellRadius = 0//CELL_RADIUS / 2.0
      for (var x = regionAABB[0][0]; x < regionAABB[1][0]; x+=CELL_RADIUS) {
        for (var y = regionAABB[0][1]; y < regionAABB[1][1]; y+=CELL_RADIUS) {
          if (!testGridCell(aworld, this, x + halfCellRadius, y + halfCellRadius)) {
            continue
          }

          if (!testGridCell(bworld, other, x + halfCellRadius, y + halfCellRadius)) {
            continue
          }

            //
            // ctx.strokeStyle = "white"
            // ctx.beginPath()
            //   ctx.moveTo(x, y)
            //   ctx.lineTo(x + CELL_RADIUS, y)
            //   ctx.lineTo(x + CELL_RADIUS, y + CELL_RADIUS)
            //   ctx.lineTo(x, y + CELL_RADIUS)
            //   ctx.lineTo(x, y)
            //   ctx.stroke();

// console.log("aworld", aworld, "bworld", bworld, 'dist', vec2.distance(aworld, bworld))


            // var isect = SAT(a, b)
            // if (isect) {
            //   ctx.restore();
            //   continue
            // }
            //
            // var r = cellResponse(a, b)
            // impulse[0] += r[0] / 2.0
            // impulse[1] += r[1] / 2.0

            var d = vec2.distance(aworld, bworld)
            if (d > CELL_RADIUS) {
              continue;
            }

          ctx.save()
            ctx.lineWidth = 1.0
            ctx.strokeStyle = "#aaa"
            ctx.beginPath()
              ctx.moveTo(aworld[0], aworld[1])
              ctx.lineTo(bworld[0], bworld[1])
              ctx.stroke()
            ctx.fillStyle = "#aaa"
            ctx.beginPath()
              ctx.arc(aworld[0], aworld[1], .2, 0, Math.PI*2)
              ctx.fill()

            var delta = d - CELL_RADIUS * 2;
            impulse[0] = (impulse[0] + (aworld[0] - bworld[0]) * -delta) / 2.0
            impulse[1] = (impulse[1] + (aworld[1] - bworld[1]) * -delta) / 2.0
          ctx.restore()

        }
      }

      if (impulse[0] || impulse[1]) {
        this.velocity[0] = this.velocity[0] * 0.55 + impulse[0] / 100.0
        this.velocity[1] = this.velocity[1] * 0.55 + impulse[1] / 100.0
      }
      // TODO: restitution
      //this.velocity[0] *= -0.1
      //this.velocity[1] *= -0.1
    }
  }

  out.tick(true)

  return out;
}

function testGridCell(out, object, x, y) {
  var hdx = object.dims[0]/2
  var hdy = object.dims[1]/2
  // TODO: these coords need to be pushed over by .5 * dims
  var grid = ([
    txo([0, 0], object.invModel, [x, y]),
    txo([0, 0], object.invModel, [x + CELL_RADIUS, y]),
    txo([0, 0], object.invModel, [x + CELL_RADIUS, y + CELL_RADIUS]),
    txo([0, 0], object.invModel, [x, y + CELL_RADIUS])
  ]).map(p => {

    p[0] = Math.floor(p[0] / CELL_RADIUS) * CELL_RADIUS
    p[1] = Math.floor(p[1] / CELL_RADIUS) * CELL_RADIUS
    return p
  })
  // console.log("grid", grid)

  var center = [
    (grid[0][0] + grid[2][0]) / 2.0,
    (grid[0][1] + grid[2][1]) / 2.0
  ]

  // console.log("center", center)

  txo(out, object.model, center)
  // console.log("out", out)
  // TODO: don't return true if the grid cell is not occupied
  return true
}

function cellResponse(a, b) {
  const ca = squareCenter(a)
  const cb = squareCenter(b)


  ctx.strokeStyle = hsl(vec2.distance(ca, cb) / CELL_RADIUS * 2)
  ctx.beginPath()

    ctx.moveTo(ca[0], ca[1])
    ctx.lineTo(cb[0], cb[1])
    ctx.arc(cb[0], cb[1], 2, 0, Math.PI*2)
    ctx.stroke()


  const ret = [
    ca[0] - cb[0],
    ca[1] - cb[1]
  ]

  return ret
}

function squareCenter(p) {
  return [
    (p[0][0] + p[2][0]) / 2,
    (p[0][1] + p[2][1]) / 2
  ]
}

const keys = {}
window.addEventListener('keydown', (e) => keys[e.code] = true)
window.addEventListener('keyup', (e) => keys[e.code] = false)

function render() {
  ctx.clear()
  camera.begin()
    center(ctx)
    ctx.scale(2.0, -2.0)
    ctx.pointToWorld(moveable.pos, camera.mouse.pos)
    moveable.rot += 0.01
    objects[2].rot += 0.01// Math.sin(Date.now() / 10000) / 1000.0
    objects.forEach((objectA, i) => {

        objectA.tick()
        objectA.render(ctx, anyIsect)
        // pairwise test between every object
        // TODO: this problem size can be reduced!!!
        var anyIsect = false
        for (var j = i+1; j<objects.length; j++) {
          var objectB = objects[j]

          var isect = SAT(
            objectA.points,
            objectB.points
          )
          anyIsect = anyIsect || isect

          if (isect) {
            var aabb = computeIsectAABB(objectA, objectB)
            objectA.renderIsect(ctx, aabb, objectB)
            //objectB.renderIsect(ctx, aabb, objectA)

            //var particlesA = collectParticles(objectA, aabb)
            var particlesB = collectParticles(objectB, aabb)
          }
        }
    })
  camera.end();
}

function collectParticles(object, aabb) {
  const hdx = object.dims[0] / 2.0
  const hdy = object.dims[1] / 2.0

  const gridAABB = [[0, 0], [0, 0]]
  txo(gridAABB[0], object.invModel, aabb[0])
  txo(gridAABB[1], object.invModel, aabb[1])
  const oaabb = [
    [ - hdx, - hdy],
    [ + hdx, + hdy]
  ]

  aabbRebuild(oaabb)
  aabbRebuild(gridAABB)

  // TODO: ensure we flip these coords if they get reversed
  const region = aabbIntersection(gridAABB, oaabb)

  ctx.strokeStyle = "red"
  aabbToGridRadius(oaabb)
  ctx.strokeRect(
    oaabb[0][0],
    oaabb[0][1],
    oaabb[1][0] - oaabb[0][0],
    oaabb[1][1] - oaabb[0][1]
  )

  ctx.strokeStyle = "orange"
  aabbToGridRadius(gridAABB)
  ctx.strokeRect(
    gridAABB[0][0],
    gridAABB[0][1],
    gridAABB[1][0] - gridAABB[0][0],
    gridAABB[1][1] - gridAABB[0][1]
  )



  ctx.strokeStyle = "green"
  aabbToGridRadius(gridAABB)
  ctx.strokeRect(
    oaabb[0][0],
    oaabb[0][1],
    oaabb[1][0] - oaabb[0][0],
    oaabb[1][1] - oaabb[0][1]
  )

  ctx.strokeStyle = "white"

  ctx.beginPath()
    square(
      object.model,
      region[0][0],
      region[0][1],
      region[1][0] - region[0][0],
      region[1][1] - region[0][1]
    )
    ctx.stroke()

  aabbToGridRadius(region)
  aabbRebuild(region)

  const cc = [0, 0]
  ctx.strokeStyle = "#fec"
  ctx.beginPath()
  for (var x = region[0][0]; x<region[1][0]; x+=CELL_RADIUS) {
    for (var y = region[0][1]; y<region[1][1]; y+=CELL_RADIUS) {
      vec2.transformMat3(
        cc,
        [x, y],
        object.model
      )
      ctx.moveTo(cc[0] + CELL_RADIUS/2.0 - 1, cc[1])
      ctx.arc(cc[0], cc[1], CELL_RADIUS/2 - 1, 0, Math.PI*2)
    }
  }
  ctx.stroke()
}

function aabbToGridRadius(aabb) {
  aabb[0][0] = Math.floor(aabb[0][0] / CELL_RADIUS) * CELL_RADIUS
  aabb[0][1] = Math.floor(aabb[0][1] / CELL_RADIUS) * CELL_RADIUS
  aabb[1][0] = Math.floor(aabb[1][0] / CELL_RADIUS) * CELL_RADIUS
  aabb[1][1] = Math.floor(aabb[1][1] / CELL_RADIUS) * CELL_RADIUS

}

function aabbRebuild(aabb) {
  var lx = aabb[0][0]
  var ly = aabb[0][1]
  var ux = aabb[1][0]
  var uy = aabb[1][1]


  aabb[0][0] = Math.min(lx, ux)
  aabb[0][1] = Math.min(ly, uy)
  aabb[1][0] = Math.max(lx, ux)
  aabb[1][1] = Math.max(ly, uy)
}

function growAABBByPoint(aabb, x, y) {
  aabb[0][0] = Math.min(aabb[0][0], x)
  aabb[0][1] = Math.min(aabb[0][1], y)
  aabb[1][0] = Math.max(aabb[1][0], x)
  aabb[1][1] = Math.max(aabb[1][1], y)
}

function computeIsectAABB(a, b) {
  const regionAABB = [[Infinity, Infinity], [-Infinity, -Infinity]]
  const aabb = aabbIntersection(a.aabb(), b.aabb())
  const aabbCenter = [aabb[1][0] - aabb[0][0], aabb[1][1] - aabb[0][1]]

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

  const region = []

  a.points.forEach((cp, pi) => {
    const cn = a.points[(pi+1) % a.points.length]

    if (rectContainsPoint(b.points, cp)) {
      growAABBByPoint(regionAABB, cp[0], cp[1])
    }

    b.points.forEach((ap, ai) => {
      const an = b.points[(ai+1) % b.points.length]

      if (rectContainsPoint(a.points, ap)) {
        growAABBByPoint(regionAABB, ap[0], ap[1])
      }

      const r = segseg(
        cp[0], cp[1], cn[0], cn[1],
        ap[0], ap[1], an[0], an[1]
      )

      if (r && r !== true) {
        // ctx.beginPath()
        //   ctx.arc(r[0], r[1], 2, 0, Math.PI*2)
        //   ctx.fill()
        growAABBByPoint(regionAABB, r[0], r[1])
      }
    })
  })


  // inflate the regionAABB to cover full grid cells (world space)
  regionAABB[0][0] = Math.floor(regionAABB[0][0] / CELL_RADIUS) * CELL_RADIUS
  regionAABB[0][1] = Math.floor(regionAABB[0][1] / CELL_RADIUS) * CELL_RADIUS
  regionAABB[1][0] = Math.ceil(regionAABB[1][0] / CELL_RADIUS) * CELL_RADIUS
  regionAABB[1][1] = Math.ceil(regionAABB[1][1] / CELL_RADIUS) * CELL_RADIUS

  // ctx.strokeStyle = "green"
  // ctx.strokeRect(
  //   regionAABB[0][0],
  //   regionAABB[0][1],
  //   regionAABB[1][0] - regionAABB[0][0],
  //   regionAABB[1][1] - regionAABB[0][1]
  // )

  return regionAABB
}

function inAABB(point, aabb) {
  return aabb[0][0] <= point[0] && aabb[0][1] <= point[1] &&
         aabb[1][0] >= point[0] && aabb[1][1] >= point[1]
}

function rectContainsPoint(points, point) {
  var c = false;
  var l = points.length
  for (var i=1; i<=l; i++) {
    var prev = points[i - 1]
    var current = points[i%l]

    ;((prev[1] <= point[1] && point[1] < current[1]) || (current[1] <= point[1] && point[1] < prev[1]))
      && (point[0] < (current[0] - prev[0]) * (point[1] - prev[1]) / (current[1] - prev[1]) + prev[0])
      && (c = !c)
  }
  return c;
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
