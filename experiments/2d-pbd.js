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
const oldPosScratch = [0, 0]
const grid = {
  array: ndarray([], [64, 64]),
  cellSize: particleRadius,
  newParticle(pos) {
    const idx = particles.length
    particles.push(pos)
    this.add(idx, pos)
    return idx
  },

  add(index) {
    const pos = particles[index]
    var v = this.cell(pos)
    if (!v) {
      v = []
      this.cell(pos, v)
    }

    v.push(index)
  },

  cell(pos, v) {
    if (v) {
      return this.array.set(
        floor(pos[0]/this.cellSize + this.array.shape[0]/2),
        floor(pos[1]/this.cellSize + this.array.shape[1]/2),
        v
      )
    } else {
      return this.array.get(
        floor(pos[0]/this.cellSize + this.array.shape[0]/2),
        floor(pos[1]/this.cellSize + this.array.shape[1]/2)
      )
    }
  },

  remove(index, pos) {
    //const pos = particles[index]
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
    const sameCell = floor(pos[0]) === floor(newPos[0]) && floor(pos[1]) === floor(newPos[1])

    oldPosScratch[0] = pos[0]
    oldPosScratch[1] = pos[1]


    pos[0] = newPos[0]
    pos[1] = newPos[1]

    if (sameCell) {
      return
    }

    this.remove(index, oldPosScratch)
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

var dims = [8, 8]
var spacing = particleRadius * 5;

const cornerDist = vec2.length([particleRadius, particleRadius])
const strength = 1.0
// for (var y = 0; y<dims[1]; y++) {
//   for (var x = 0; x<dims[0]; x++) {
//
//     var l = particles.length;
//     grid.newParticle([
//       x*spacing + spacing/2,
//       y*spacing + spacing/2
//     ])
//
//     // constraints.push({
//     //   pair: [l-1, l],
//     //   type: 'distance',
//     //   value: particleRadius,
//     //   strength: strength
//     // })
//     //
//     // constraints.push({
//     //   pair: [l-dims[0], l],
//     //   type: 'distance',
//     //   value: particleRadius,
//     //   strength: strength
//     // })
//     //
//     // constraints.push({
//     //   pair: [l+dims[0] + 1, l],
//     //   type: 'distance',
//     //   value: particleRadius,
//     //   strength: strength
//     // })
//   }
// }

grid.newParticle([-spacing, 0])
grid.newParticle([0, 0])
grid.newParticle([spacing, 0])

constraints.push({
  pair: [1, 0, 2],
  type: 'pointOnLine',
  value: particleRadius,
  stiffness: strength* 2,
})

// add a lattice of constraints
particles.forEach((p, i) => {
  constraints.push({
    pair: [i-1, i],
    type: 'distance',
    value: particleRadius*2,
    compressionStiffness: strength,
    stretchStiffness: strength
  })

  // constraints.push({
  //   pair: [i+1, i],
  //   type: 'distance',
  //   value: particleRadius,
  //   compressionStiffness: strength,
  //   stretchStiffness: strength
  // })
  //
  // constraints.push({
  //   pair: [i-dims[0], i],
  //   type: 'distance',
  //   value: particleRadius,
  //   compressionStiffness: strength,
  //   stretchStiffness: strength
  // })
  //
  // constraints.push({
  //   pair: [i+dims[0], i],
  //   type: 'distance',
  //   value: particleRadius,
  //   compressionStiffness: strength,
  //   stretchStiffness: strength
  // })
  // //
  // constraints.push({
  //   pair: [i+dims[0] + 1, i],
  //   type: 'distance',
  //   value: cornerDist,
  //   compressionStiffness: strength,
  //   stretchStiffness: strength
  // })
  // //
  // constraints.push({
  //   pair: [i+dims[0] - 1, i],
  //   type: 'distance',
  //   value: cornerDist,
  //   compressionStiffness: strength,
  //   stretchStiffness: strength
  // })
  // // //
  // // //
  // constraints.push({
  //   pair: [i-dims[0] + 1, i],
  //   type: 'distance',
  //   value: cornerDist,
  //   compressionStiffness: strength,
  //   stretchStiffness: strength
  // })
  // constraints.push({
  //   pair: [i-dims[0] - 1, i],
  //   type: 'distance',
  //   value: cornerDist,
  //   compressionStiffness: strength,
  //   stretchStiffness: strength
  // })
})


// const centerOfMass = [(dims[0]*spacing)/2, (dims[1] * spacing)/2]
// const centerIdx = grid.newParticle(centerOfMass)
// for (var i=0; i<particles.length-1; i++) {
//   var a = particles[i]
//   constraints.push({
//     pair: [i, centerIdx],
//     type: 'distance',
//     value: vec2.distance(a, centerOfMass),
//     compressionStiffness: 1.5,
//     stretchStiffness: 1.0
//   })
// }


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

    // TODO: currently we only support particles with mass=1
    const massSum = 2.0;

    const d = vec2.distance(a, b)

    // TODO: we should probably return some sort of encoding here so we can
    //       constrain against multiple things, but for the time being we'll
    //       just blindly apply a transformation immediately in a pairwise
    //       fashion

    var delta = 0.0
    if (d < constraint.value) {
  		delta = constraint.compressionStiffness * (d - constraint.value) / massSum;
    } else {
  		delta = constraint.stretchStiffness * (d - constraint.value) / massSum;
    }

    // TODO: this is where we'd apply the mass
    vec2.subtract(v2scratch, a, [dir[0] * delta, dir[1] * delta])
    grid.moveParticle(constraint.pair[0], v2scratch)

    vec2.add(v2scratch, b, [dir[0] * delta, dir[1] * delta])
    grid.moveParticle(constraint.pair[1], v2scratch)

  },
  fixed(c) {
    grid.moveParticle(c.pair[0], c.value)
  },
  // point on line works
  pointOnLine(c) {
    const point = particles[c.pair[0]]
    const start = particles[c.pair[1]]
    const end = particles[c.pair[2]]

    const px = point[0]
    const py = point[1]
    const x1 = start[0]
    const y1 = start[1]
    const x2 = end[0]
    const y2 = end[1]
    const stiffness = c.stiffness

    var dx = x2 - x1;
    var dy = y2 - y1;

    var m = dy / dx; //Slope
    var n = dx / dy; //1/Slope

    var dx = 0.0;
    var dy = 0.0;

    if(m <= 1 && m >= -1) {
      //Calculate the expected y point given the x coordinate of the point
      const y = y1 + m * (px - x1);
      dx = y - py;
      v2scratch[0] = y
    } else {
      const x = x1 + n * (py - y1);
      var dx = x - px;
      v2scratch[0] = x
    }

    // TODO: in order for this to work we have to move each point to be
    //       on the line. I think the two endpoints move less than the center

    // v2scratch[0] = point[0] + dx// * 0.75
    // v2scratch[1] = point[1] + dy// * 0.75
    grid.moveParticle(c.pair[0], v2scratch)

    // v2scratch[0] = start[0] + dx * 0.125
    // v2scratch[1] = start[1] + dy * 0.125
    // grid.moveParticle(c.pair[1], v2scratch)
    //
    // v2scratch[0] = end[0] + dx * 0.125
    // v2scratch[1] = end[1] + dy * 0.125
    // grid.moveParticle(c.pair[2], v2scratch)
  }
}

function collectContactConstraints() {
  var contactConstraints = []
  for (var i=0; i<particles.length; i++) {
    for (var j=i+1; j<particles.length; j++) {
      contactConstraints.push({
        pair: [i, j],
        type: 'distance',
        value: particleRadius,
        compressionStiffness: 1.0,
        stretchStiffness: 0
      })
    }
  }

  // TODO: this is only slightly more optimal, but breaks due to not looking at
  //       neighboring cells
  // var contactConstraints = []
  // for (var x=0; x<grid.array.shape[0]; x++) {
  //   for (var y=0; y<grid.array.shape[1]; y++) {
  //     var v = grid.array.get(x, y)
  //     if (!v || !v.length) {
  //       continue
  //     }
  //
  //     // TODO: this is a stupid slow hack
  //     for (var i=0; i<v.length; i++) {
  //       var firstIdx = v[i]
  //       for (var j=i+1; j<v.length; j++) {
  //         contactConstraints.push({
  //           pair: [firstIdx, v[j]],
  //           type: 'distance',
  //           value: particleRadius,
  //           compressionStiffness: 1.0,
  //           stretchStiffness: 0
  //         })
  //       }
  //     }
  //   }
  // }
  return contactConstraints
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


    collectContactConstraints().forEach((c) => {
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
  step(50)

  camera.begin()
    center(ctx)
    ctx.scale(2.0, 2.0)

    ctx.pointToWorld(mouse, camera.mouse.pos)

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

          // if (i == centerIdx) {
          //   ctx.fillStyle = "yellow"
          // } else {
            ctx.fillStyle = hoveredParticle == i ? 'red' : hsl(i/particles.length * 0.9)
          // }
          ctx.fill();
      })

      constraints.forEach((c, i) => {
        if (c.pair == 1) {
          return
        }
        const a = particles[c.pair[0]]
        const b = particles[c.pair[1]]
        ctx.strokeStyle = hsl(i/particles.length * 0.9)
        // ctx.beginPath()
        //   ctx.moveTo(a[0], a[1])
        //   ctx.lineTo(b[0], b[1])
        //   ctx.stroke()
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
