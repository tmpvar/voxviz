const { mat3, vec2 } = require('gl-matrix')
const ndarray = require('ndarray')

const createTickBuffer = require('./tickbuffer')

module.exports = createBody

const points = [[0, 0],[0, 0],[0, 0],[0, 0]]

function createBody(pos, dims, density) {

  dims[0] = Math.ceil(dims[0])
  dims[1] = Math.ceil(dims[1])


  density = density || 1
  const area = dims[0] * dims[1]
  const mass = area * density
  const invMass = 1.0 / mass
  const momentOfInertia = mass * area
  const invMomentOfInertia = (momentOfInertia !== 0) ? 1.0 / momentOfInertia : 0.0

  const body = {
    mass: mass,
    invMass: invMass,
    momentOfInertia: momentOfInertia,
    invMomentOfInertia: invMomentOfInertia,
    restitution: 1.0,

    history: createTickBuffer(1, o => {
      return {
        pos: [pos[0], pos[1]],
        rot: 0,
        posVelocity: [0, 0],
        rotVelocity: 0,
        model: mat3.create(),
        invModel: mat3.create()
      }
    }, (p, c, dt) => {
      c.pos[0] = p.pos[0] + p.posVelocity[0] * dt
      c.pos[1] = p.pos[1] + p.posVelocity[1] * dt
      c.rot = p.rot + p.rotVelocity * dt

      mat3.identity(c.model)
      mat3.translate(c.model, c.model, c.pos)
      mat3.rotate(c.model, c.model, c.rot)
      mat3.invert(c.invModel, c.model)

      return c
    }),

    get current() {
      return this.history.value()
    },

    data: ndarray(new Uint8Array(dims[0] * dims[1]), dims),
    dims: dims,
    tick(dt) {
      dt = dt || 1
      this.history.tick(dt)
    },

    render(ctx) {
      const hdx = this.dims[0] / 2
      const hdy = this.dims[1] / 2

      const model = this.current.model
      // draw grid
      ctx.strokeStyle = "#666"

      vec2.set(points[0], -hdx, -hdy)
      vec2.transformMat3(points[0], points[0], model)

      vec2.set(points[1], hdx, -hdy)
      vec2.transformMat3(points[1], points[1], model)

      vec2.set(points[2], hdx, hdy)
      vec2.transformMat3(points[2], points[2], model)

      vec2.set(points[3], -hdx, hdy)
      vec2.transformMat3(points[3], points[3], model)

      ctx.strokeStyle = "white" //isect ? "red" : "#aaa"
      ctx.beginPath()
        ctx.moveTo(points[0][0], points[0][1])
        ctx.lineTo(points[1][0], points[1][1])
        ctx.lineTo(points[2][0], points[2][1])
        ctx.lineTo(points[3][0], points[3][1])
        ctx.lineTo(points[0][0], points[0][1])
        ctx.stroke();

      ctx.fillStyle = "white"
      ctx.beginPath()
      points.forEach(p => {
        ctx.moveTo(p[0], p[1])
        ctx.arc(p[0], p[1], 0.3, 0, Math.PI*2, false)
        ctx.fill()
      })
    }
  }

  return body
}
