const { vec2 } = require('gl-matrix')

module.exports = createSim

function crossScalar(out, n, v) {
  out[0] =  n * v[1]
  out[1] = -n * v[0]
  return out
}

function crossVec(v1, v2) {
  return v1[0] * v2[1] - v1[1] * v2[0]
}

function dot(v1, v2) {
  return v1[0] * v2[0] + v1[1] * v2[1]
}


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
        body.current.posVelocity[1] -= 9.8*dt
        this.rasterizeBody(body, bodyId, contacts)
      })

      return contacts
    },

    rasterizeBody(body, bodyId, contacts) {
      const dims = body.dims;
      const hdx = dims[0] / 2.0
      const hdy = dims[1] / 2.0
      const p = [0, 0]
      const pp = [0, 0]
      const cell = [0, 0]
      for (var x = 0; x<=dims[0]; x++) {
        for (var y = 0; y<=dims[1]; y++) {
          p[0] = (x - hdx)
          p[1] = (y - hdy)

          vec2.transformMat3(p, p, body.current.model)

          p[0] = Math.floor(p[0])
          p[1] = Math.floor(p[1])
          if (p[0] < 0 || p[1] < 0 || p[0] >= grid.dims[0] || p[1] >= grid.dims[1]) {
            continue;
          }

          var contact = grid.set(p, bodyId)

          if (contact !== false) {
            cell[0] = p[0]
            cell[1] = p[1]

            contacts.push({
              a: bodyId,
              b: contact,
              cell: cell,
              pos: [p[0], p[1]],
              local: [
                x - hdx,
                y - hdy
              ],
              grid: [x, y]
            })
          }
        }
      }
    },

    computeCollisionResponse(contacts, dt) {
      // Relative velocity
      const rv = [0, 0]
      const ra = [0, 0]
      const rb = [0, 0]
      const normal = [0, 0]
      const impulse = [0, 0]
      const cellCenter = [0, 0]
      const scratch0 = [0, 0]
      const scratch1 = [0, 0]

      contacts.forEach(c => {
        const a = this.bodies[c.a]
        const acur = a.current
        const aprev = a.history.value(-1)

        const b = this.bodies[c.b]
        const bcur = b.current
        const bprev = b.history.value(-1)

        // vec2.set(cellCenter,
        //   c.cell[0] + grid.cellRadius / 2.0,
        //   c.cell[1] + grid.cellRadius / 2.0
        // )

        //vec2.subtract(normal, bcur.pos, acur.pos)
        // if (vec2.distance(c.pos, acur.pos) < vec2.distance(cellCenter, acur.pos)) {
        //   vec2.subtract(normal, c.pos, c.cell)
        // } else {
        vec2.subtract(normal, c.pos, c.cell)

        if (Math.abs(normal[0]) > Math.abs(normal[1])) {
          normal[0] -= Math.sign(normal[0])
          normal[1] = 0
        } else {
          normal[0] = 0
          normal[1] -= Math.sign(normal[1])
        }

        vec2.normalize(normal, normal)
        // normal[0] = Math.sign(c.pos[0] - cellCenter[0])
        // normal[1] = Math.sign(c.pos[1] - cellCenter[1])

        // Calculate radii from COM to contact
        vec2.subtract(ra, c.pos, acur.pos);
        vec2.subtract(rb, c.pos, bcur.pos);

        // a.posVelocity + Cross( B->angularVelocity, rb ) -
        //           a.posVelocity - Cross( A->angularVelocity, ra );
        vec2.subtract(rv,
          vec2.add(scratch0,
            bcur.posVelocity,
            crossScalar(scratch0, bcur.rotVelocity, rb)
          ),
          vec2.subtract(scratch1,
            acur.posVelocity,
            crossScalar(scratch1, acur.rotVelocity, ra)
          )
        )

        // Relative velocity along the normal
        var contactVel = vec2.dot(rv, normal);

        // Do not resolve if velocities are separating
        if(contactVel > 0) {
          //console.log('skip', c.cell, c.pos)
          //return
          normal[0] = -normal[0]
          normal[1] = -normal[1]
          contactVel = -contactVel
          // normal[0] = rv[0]
          // normal[1] = rv[1]
          // contactVel = vec2.dot(rv, normal);
        }

        const raCrossN = crossVec(ra, normal);
        const rbCrossN = crossVec(rb, normal);

        const invMassSum = a.invMass + b.invMass +
          Math.pow(raCrossN, 2) * a.invMomentOfInertia +
          Math.pow(rbCrossN, 2) * b.invMomentOfInertia

        const restitution = Math.min(a.restitution, b.restitution)

        // Calculate impulse scalar
        var j = -(1.0 + 0.1) * contactVel
        j /= invMassSum
        // TODO: divide by the number of contacts affecting this pair
        // j /= 10
        //j /= (real)contact_count;
// console.log(j, restitution, contactVel, invMassSum)
        // Apply impulse
        /*

        velocity += invMass * impulse;
        angularVelocity += invMomentOfInertia * cross( contactVector, impulse );
        */

        vec2.scale(impulse, normal, j);
        if (isNaN(impulse[0]) || isNaN(impulse[1])) {
          debugger
        }

        applyImpulse(a, [-impulse[0], -impulse[1]], ra)
        applyImpulse(b, impulse, rb)
      })

      function applyImpulse(body, impulse, contactVector) {
        body.current.posVelocity[0] += body.invMass * impulse[0];
        body.current.posVelocity[1] += body.invMass * impulse[1];
        body.current.rotVelocity += body.invMomentOfInertia * crossVec( contactVector, impulse );
      }
    }
  }


  return sim
}
