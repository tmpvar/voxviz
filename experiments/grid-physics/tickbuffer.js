

module.exports = createTickBuffer

function createTickBuffer(size, init, update) {

  const buffer = []
  for (var i=0; i<size; i++) {
    var obj = {}
    buffer.push(init(obj) || obj)
  }

  const tb = {
    buffer: buffer,
    tickIdx: 0,

    // move the current slot over by one and initialize the current slot
    tick(dt) {
      const p = this.value()
      this.tickIdx = (this.tickIdx+1)%size
      const c = this.value()

      this.buffer[this.tickIdx] = update(p, c, dt) || c
    },

    value(offset) {
      offset = offset || 0
      var idx = (this.tickIdx + offset) % size
      if (idx < 0) {
        idx += size
      }
      return this.buffer[idx]
    }
  }

  return tb
}
