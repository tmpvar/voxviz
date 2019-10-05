const { vec2 } = require('gl-matrix')

module.exports = createSim

function createSim(grid) {
  if (!grid) {
    return
  }

  const sim = {
    bodies: [],
    time: 1,
    addBody(body) {
      this.bodies.push(body)
    },

    tick(dt) {
      this.time++
      grid.tick(this.time)
      var contacts = []
      this.bodies.forEach((body, bodyId) => {
        body.tick(dt, this.time)
        this.rasterizeBody(body, bodyId, contacts)
      })

      return contacts
    },

    rasterizeBody(body, bodyId, contacts) {
      const dims = body.dims;
      const hdx = dims[0] / 2.0
      const hdy = dims[0] / 2.0
      const p = [0, 0]
      for (var x = 0; x<dims[0]; x++) {
        for (var y = 0; y<dims[1]; y++) {
          p[0] = (x - hdx + 0.5)
          p[1] = (y - hdy + 0.5)

          vec2.transformMat3(p, p, body.current.model)

          var contact = grid.set(p, bodyId)

          if (contact !== false) {
            contacts.push({
              a: bodyId,
              b: contact,
              cell: [p[0]|0, p[1]|0],
              pos: [p[0], p[1]]
            })
          }
        }
      }
    },

    // TODO: this probably moves into tick()
    computeCollisionResponse(contacts) {

      contacts.forEach(c => {
        const a = this.bodies[c.a]
        const acur = a.current

        const b = this.bodies[c.b]
        const bcur = b.current

        const p = c.cell

        const dx = acur.pos[0] - p[0]
        const dy = acur.pos[1] - p[1]

        acur.posVelocity[0] += dx / 2.0
        acur.posVelocity[1] += dy / 2.0

        bcur.posVelocity[0] -= dx / 2.0
        bcur.posVelocity[1] -= dy / 2.0
      })
    }
  }


  return sim
}
