const ctx = require('fc')(render, 1)
const center = require('ctx-translate-center')
const glmatrix = require('gl-matrix')
const renderGrid = require('ctx-render-grid-lines')
const vec2 = glmatrix.vec2
const mat3 = glmatrix.mat3
const segseg = require('segseg')
const ndarray = require('ndarray')
const raySlab = require('ray-aabb-slab')


const particles = []
const constraints = []
var dragConstraint = null

const particleRadius = 10.0;
const mouse = [0, 0];
var hoveredParticle = -1
var dragParticle = null
const floor = Math.floor
const grid = {
  array: ndarray([], [1024, 1024]),
  cellSize: particleRadius,
  newParticle(pos) {
    const idx = particles.length
    particles.push(pos)
    this.add(idx, pos)
  },

  add(index) {
    const pos = particles[index]
    const v = this.cell(pos) || []
    v.push(index)
  },

  cell(pos) {
    return this.array.get(
      floor(pos[0]/this.cellSize),
      floor(pos[1]/this.cellSize)
    )
  },

  remove(index) {
    const pos = particles[index]
    const v = this.cell(pos)
    if (!v) {
      return
    }
    const idx = v.indexOf(index)
    if (!Array.isArray(v) || idx === -1) {
      return
    }

    v.splice(idx, 1)
  },

  moveParticle(index, newPos) {
    const pos = particles[index]
    if (floor(pos[0]) === floor(newPos[0]) && floor(pos[1]) === floor(newPos[1])) {
      pos[0] = newPos[0]
      pos[1] = newPos[1]

      return
    }

    pos[0] = newPos[0]
    pos[1] = newPos[1]

    this.remove(index)
    this.add(index)
  }
}

const camera = require('ctx-camera')(ctx, window, {
  mousemove(e, defaultFn) {
    // make sure we keep tracking this.mouse state!
    defaultFn(e)
    if (!dragParticle) {
      return
    }

    // particles[dragParticle.id][1] = mouse[1] + dragParticle.offset[1]
    // particles[dragParticle.id][0] = mouse[0] + dragParticle.offset[0]
    dragConstraint.value[0] = mouse[0] + dragParticle.offset[0]
    dragConstraint.value[1] = mouse[1] + dragParticle.offset[1]
  },

  mousedown(e, defaultFn) {
    if (hoveredParticle < 0) {
      return defaultFn(e)
    }

    const p = particles[hoveredParticle]

    dragParticle = {
      id: hoveredParticle,
      offset: [
        p[0] - mouse[0],
        p[1] - mouse[1]
      ]
    }

    dragConstraint = {
      pair: [hoveredParticle],
      type: 'fixed',
      value: [mouse[0], mouse[1]],
      strenth: 10000
    }
  },

  mouseup(e, defaultFn) {
    dragParticle = null
    dragConstraint = null
    defaultFn(e)
  }
})

const TAU = Math.PI*2

const lineTo = ctx.lineTo.bind(ctx)
const moveTo = ctx.moveTo.bind(ctx)
const arc = ctx.arc.bind(ctx)
const translate = ctx.translate.bind(ctx)

const view = mat3.create()
const model = mat3.create()
const invModel = mat3.create()
const v2tmp = vec2.create()
const v2tmp2 = vec2.create()
const v2dir = vec2.create()
const v2offset = vec2.create()
const v2gridPos = vec2.create()

var dims = [30, 2]

const cornerDist = vec2.length([particleRadius, particleRadius])
const strength = 10
for (var y = 0; y<dims[1]; y++) {
  for (var x = 0; x<dims[0]; x++) {

    var l = particles.length;
    grid.newParticle([x*particleRadius, y*particleRadius])

    // constraints.push({
    //   pair: [l-1, l],
    //   type: 'distance',
    //   value: particleRadius,
    //   strength: strength
    // })
    //
    // constraints.push({
    //   pair: [l-dims[0], l],
    //   type: 'distance',
    //   value: particleRadius,
    //   strength: strength
    // })
    //
    // constraints.push({
    //   pair: [l+dims[0] + 1, l],
    //   type: 'distance',
    //   value: particleRadius,
    //   strength: strength
    // })
  }
}

// add a lattice of constraints
particles.forEach((p, i) => {
  constraints.push({
    pair: [i-1, i],
    type: 'distance',
    value: particleRadius,
    strength: strength
  })

  // constraints.push({
  //   pair: [i+1, i],
  //   type: 'distance',
  //   value: particleRadius,
  //   strength: strength
  // })
  //
  // constraints.push({
  //   pair: [i-dims[0], i],
  //   type: 'distance',
  //   value: particleRadius,
  //   strength: strength
  // })
  //
  // constraints.push({
  //   pair: [i+dims[0], i],
  //   type: 'distance',
  //   value: particleRadius,
  //   strength: strength
  // })
  //
  // constraints.push({
  //   pair: [i+dims[0] + 1, i],
  //   type: 'distance',
  //   value: cornerDist,
  //   strength: strength
  // })
  //
  // constraints.push({
  //   pair: [i+dims[0] - 1, i],
  //   type: 'distance',
  //   value: cornerDist,
  //   strength: strength
  // })
  // //
  // //
  // constraints.push({
  //   pair: [i-dims[0] + 1, i],
  //   type: 'distance',
  //   value: cornerDist,
  //   strength: strength
  // })
  // constraints.push({
  //   pair: [i-dims[0] - 1, i],
  //   type: 'distance',
  //   value: cornerDist,
  //   strength: strength
  // })

})

const v2scratch = [0, 0]
const constraintMap = {
  distance(constraint) {
    const a = particles[constraint.pair[0]]
    const b = particles[constraint.pair[1]]
    if (!a || !b) {
      return
    }
    const dir = [0, 0]
    vec2.normalize(dir, vec2.subtract(dir, a, b))

    const d = vec2.distance(a, b)

    // TODO: we should probably return some sort of encoding here so we can
    //       constrain against multiple things, but for the time being we'll
    //       just blindly apply a transformation immediately in a pairwise
    //       fashion

    delta = (d - constraint.value) / 2.0

    vec2.subtract(v2scratch, a, [dir[0] * delta, dir[1] * delta])
    grid.moveParticle(constraint.pair[0], v2scratch)

    vec2.add(v2scratch, b, [dir[0] * delta, dir[1] * delta])
    grid.moveParticle(constraint.pair[1], v2scratch)
  },
  fixed(c) {
    grid.moveParticle(c.pair[0], c.value)
  }
}

function step(iters) {
  iters = iters || 5;
  for (var i=0; i<iters; i++) {
    constraints.forEach((c) => {
      if (!constraintMap[c.type]) {
        return console.warn("constraint type", c.type, "not defined")
      }

      constraintMap[c.type](c)
    })

    if (dragConstraint && constraintMap[dragConstraint.type]) {
      constraintMap[dragConstraint.type](dragConstraint)
    }
  }
}


function render() {
  ctx.clear()
  camera.begin()
    center(ctx)
    ctx.scale(2.0, 2.0)

    ctx.pointToWorld(mouse, camera.mouse.pos)
    step(10)
    ctx.save()
      ctx.lineWidth = 0.05

      ctx.fillStyle = "red"
      hoveredParticle = -1
      particles.forEach((p, i) => {
        ctx.beginPath()
          ctx.arc(p[0], p[1], particleRadius / 2.0, 0, Math.PI*2)

          if (vec2.distance(mouse, p) <= 5) {
            hoveredParticle = i
          }
          ctx.fillStyle = hoveredParticle == i ? 'red' : 'white'
          ctx.fill();
      })

    ctx.restore()
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
