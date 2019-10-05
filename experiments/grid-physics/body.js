const { mat3, vec2 } = require('gl-matrix')
const ndarray = require('ndarray')

const createTickBuffer = require('./tickbuffer')

module.exports = createBody

function createBody(pos, dims) {
  const body = {
    history: createTickBuffer(2, o => {
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
  }




  return body
}
