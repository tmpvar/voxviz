const ndarray = require('ndarray')

module.exports = createGrid
function createGrid(dims, cellRadius) {
  cellRadius = cellRadius || 1
  
  const grid = {
    dims: dims,
    data: ndarray([], dims),
    time: 1,

    set(pos, bodyId, timeDelta) {
      timeDelta = timeDelta || 1
      const x = (pos[0] / cellRadius)|0
      const y = (pos[1] / cellRadius)|0

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
      const x = (pos[0] / cellRadius)|0
      const y = (pos[1] / cellRadius)|0

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
    }
  }

  return grid
}
