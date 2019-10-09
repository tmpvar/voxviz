const ndarray = require('ndarray')
const fill = require('ndarray-fill')
const glmatrix = require('gl-matrix')

const createOccupancyGrid = require('./grid')
const createObject = require('./body')
const createSim = require('./sim')

const vec2 = glmatrix.vec2
const mat3 = glmatrix.mat3

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

function grey(p) {
  return `rgba(255, 255, 255, ${p})`
}

const grid = createOccupancyGrid([100, 100], 1)

const sim = createSim(grid)

const moveable = createObject([105, 10], [40, 10])
// moveable.current.rot = Math.PI/4
sim.addBody(moveable)

const t = createObject([85, 20], [5, 20])
// moveable.current.rot = Math.PI/4
sim.addBody(t)


function addRandomObject() {
  var o = createObject(
    [(0.001 + Math.random()) * 100, 100],
    [1 + (0.001 + Math.random()) * 10, 1 + Math.random() * 10],
  )
  o.rot = Math.random() * Math.PI*2
  sim.addBody(o)
}

const ctx = require('fc')(render, 1)
const center = require('ctx-translate-center')
const renderGrid = require('ctx-render-grid-lines')

const camera = require('ctx-camera')(ctx, window, {})

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

setInterval(() =>{
  if (keys.KeyA) {
    addRandomObject()
  }
}, 40)

const keys = {}
window.addEventListener('keydown', (e) => keys[e.code] = true)
window.addEventListener('keyup', (e) => keys[e.code] = false)

var lastTime = Date.now()
function render() {
  var now = Date.now()
  var dt = now - lastTime
  lastTime = now

  ctx.clear()
  camera.begin()
    ctx.scale(CELL_RADIUS, -CELL_RADIUS)
    ctx.lineWidth = 0.3
    ctx.translate(10, -window.innerHeight / 11.)
    const oldPos = [moveable.current.pos[0], moveable.current.pos[1]]
    ctx.pointToWorld(moveable.current.pos, camera.mouse.pos)
    moveable.current.posVelocity[0] = moveable.current.pos[0] - oldPos[0]
    moveable.current.posVelocity[1] = moveable.current.pos[1] - oldPos[1]
    moveable.current.rotVelocity = 0

    var contacts = sim.tick(dt/1000)

    sim.computeCollisionResponse(contacts, dt/1000)
    grid.render(ctx, 1)
    sim.bodies.forEach((obj, i) => {
      obj.render(ctx)
    })


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
