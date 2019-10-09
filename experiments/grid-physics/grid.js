const ndarray = require('ndarray')

module.exports = createGrid
function createGrid(dims, cellRadius) {
  cellRadius = 1//cellRadius || 1

  const grid = {
    dims: dims,
    data: ndarray([], dims),
    get cellRadius() { return cellRadius },
    time: 1,

    set(pos, bodyId, timeDelta) {
      timeDelta = timeDelta || 1
      const x = pos[0]|0
      const y = pos[1]|0

      var v = this.data.get(x, y)
      if (v && v.object !== bodyId && this.time - v.time < timeDelta) {
        return v.object
      }
      v = v || {}
      v.time = this.time
      v.object = bodyId

      this.data.set(x, y, v)
      return false
    },

    get(pos, objectId) {
      const x = pos[0]|0
      const y = pos[1]|0

      return this.data.get(x, y)
    },

    tick() {
      this.time++
    },

    print() {
      const dims = this.dims

      process.stdout.write('\n')
      for (var x = 0; x<dims[0]; x++) {
        process.stdout.write(x + '     ')
      }
      process.stdout.write('\n\n')


      for (var y = 0; y<dims[1]; y++) {
        if (y > 0) {
          process.stdout.write(y + ' ')
        } else {
          process.stdout.write('  ')
        }

        for (var x = 0; x<dims[0]; x++) {
          var v = this.data.get(x, y)
          if (v) {
            process.stdout.write(v.object + '@' + v.time + ' ')
          } else {
            process.stdout.write('     ')
          }
        }
        process.stdout.write('\n\n')
      }
    },

    render(ctx, timeDelta) {
      ctx.save()
      ctx.lineWidth = 0.1
      ctx.strokeStyle = "#444"
      for (var x=0; x<dims[0]; x++) {
        for (var y=0; y<dims[1]; y++) {
          ctx.strokeRect(
            x + .1,
            y + .1,
            .98,
            .98
          )

          var v = this.data.get(x, y)
          if (v) {
            var dt = this.time - v.time
            if (dt < timeDelta) {
              ctx.fillStyle = hsl(1.0 - Math.min(1.0, 1.0 - dt/timeDelta) * 0.7)
              ctx.fillRect(
                x + .2,
                y + .2,
                0.9,
                0.9
              )
            }
          }
        }
      }
      ctx.restore()
    }
  }

  return grid
}

function hsl(p, a) {
  return `hsla(${p*360}, 100%, 46%, ${a||1})`
}
