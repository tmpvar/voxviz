const ndarray = require('ndarray')
const fill = require('ndarray-fill')
const tape = require('tape')
const chalk = require('chalk')

const radius = 8
const levels = 3

function mix(x, y, a) {
  return x * (1-a) + y * a
}

function createCascade(radius, levelCount) {
  const cascades = []
  for (var levelIdx = 0; levelIdx<levelCount; levelIdx++) {
    cascades.push({
      size: 1 << (levelIdx + 1),
      data: new Uint8Array(radius)
    })
  }


  const that = {
    set(pos, v) {
      for (var level=0; level<levelCount; level++) {
        var cellSize = 1 << (level + 1)
        var cell = pos >> (level + 1)
        if (cell < radius) {
          cascades[level].data[cell] = v
        }
      }
      return this
    },

    get(pos, level) {
      if (level >= levelCount) {
        return 0
      }
      var cellSize = 1 << (level + 1)
      var cell = pos >> (level + 1)
      const v = cascades[level].data[cell]
      return v === undefined ? -1 : v
    },

    getCell(cell, level) {
      return cascades[level].data[cell]
    },

    //  0 1 2 3 4 5 6 7
    // | | | | | | | |x|
    //
    //   0   1   2   3   4   5   6   7
    // |   |   |   | x |   |   |   | x |
    //
    //     0       1       2       3       4      5       6        7
    // |       |   x   |       |   x   |       |       |   x   |       |
    print() {
      for (var level=0; level<levelCount; level++) {
        var cascade = cascades[level]
        var indices = [0, 1, 2, 3, 4, 5, 6, 7, 8]
        var spacing = 1 << (level + 1)
        console.log(indices.join((' ').repeat(spacing - 1)))
        var values = [];
        for (var i=0; i<radius; i++) {
          values.push('|' + (' ').repeat(spacing/2 - 1) + (cascade.data[i] ? 'x' : ' ') + (' ').repeat(spacing/2 - 1))
        }
        console.log(values.join('') + '|')
      }
      return this
    },

    traverse(fn) {
      var sentinal = 100
      var step = 0;
      while (sentinal --) {
        const state = this.iterator.next()
        if (state.pos >= (radius) * (1<<levelCount)) {
          break
        }

        if (fn && fn(state, step)) {
          break
        }

        step++
      }
      return this
    }
  }


  that.iterator = {
    state: {
      level: levels - 1,
      pos: 0,
      res: 0
    },
    pos(v) {
      this.state.pos = pos;
      this.state.cell = pos >> (this.state.level + 1)
    },
    reset() {
      this.state.level = levels - 1
      this.state.pos = 0
    },

    next() {
      const cellSize = 1 << (this.state.level + 1)
      const lowerCellSize = 1 << (this.state.level)

      const canGoUp = this.state.level < levels - 1
      const canGoDown = this.state.level > 0 && this.state.pos < radius * lowerCellSize
      const canStep = this.state.pos < cellSize * radius

      const upperValue = canGoUp && that.get(this.state.pos, this.state.level + 1)
      const currentValue = that.get(this.state.pos, this.state.level)

      this.state.res = currentValue

      const ret = this.getState()

      const levelChange = currentValue > 0
      ? -(canGoDown|0)
      : (canGoUp && upperValue < 1)|0

      const stepChange = (canStep && levelChange === 0 && (upperValue > 0 || !canGoUp))|0
      this.state.level += levelChange
      this.state.pos += stepChange * cellSize
      return ret
    },

    getState() {
      return {
        level: this.state.level,
        pos: this.state.pos,
        cell: this.state.pos >> (this.state.level + 1),
        res: this.state.res,
        cellSize: 1 << (this.state.level + 1)
      }
    }
  }

  return that
}

tape('fetch', (t) => {
  const c = createCascade(8, 3).set(63, 1)

  ;([0, 0, 0, 0, 0, 0, 0, 1]).forEach((expect, cell) => {
    t.equal(expect, c.getCell(cell, 2))
  })

  t.equal(1, c.get(62, 2), 'position is occupied')
  t.equal(0, c.get(54, 2), 'position is not occupied')
  t.end()
})

tape('empty', (t) => {
  const c = createCascade(8, 3)
  var next = 0
  c.traverse((state) => {
    if (state.res !== 0) {
      t.fail("should never")
    }
    t.equal(state.pos, next)
    next = Math.min(next + (1 << 3), 8 << 3)
  })
  t.end()
})

tape('walk largest', (t) => {
  const c = createCascade(8, 3).set(63, 1)
    const expect = [
    { level: 2, pos: 0,  cell: 0, res:  0 },
    { level: 2, pos: 8,  cell: 1, res:  0 },
    { level: 2, pos: 16,  cell: 2, res:  0 },
    { level: 2, pos: 24,  cell: 3, res:  0 },
    { level: 2, pos: 32,  cell: 4, res:  0 },
    { level: 2, pos: 40,  cell: 5, res:  0 },
    { level: 2, pos: 48,  cell: 6, res:  0 },
    { level: 2, pos: 56,  cell: 7, res:  1 },
  ]

  c.print()
  var ok = true
  c.traverse((state, i) => {
    ok = ok && diffObject(expect[i], state)
    ok = ok && i < expect.length
  })
  t.ok(ok, "path matches")
  t.end()
})

tape('set: 33', (t) => {
  const c = createCascade(8, 3).set(33, 1)
  const expect = [
    { level: 2, pos: 0,  cell: 0, res:  0 },
    { level: 2, pos: 8,  cell: 1, res:  0 },
    { level: 2, pos: 16,  cell: 2, res:  0 },
    { level: 2, pos: 24,  cell: 3, res:  0 },
    { level: 2, pos: 32,  cell: 4, res:  1 },
    { level: 2, pos: 40,  cell: 5, res:  0 },
    { level: 2, pos: 48,  cell: 6, res:  0 },
    { level: 2, pos: 56,  cell: 7, res:  0 },
  ]

  c.print()
  var ok = true
  c.traverse((state, i) => {
    ok = ok && diffObject(expect[i], state)
  })
  t.ok(ok, "path matches")
  t.end()
})

tape('set: 4, 20', (t) => {
  const c = createCascade(8, 3).set(20, 1).set(4, 1)
  const expect = [
    { level: 2, pos: 0,  cell: 0, res:  1 },
    { level: 1, pos: 0,  cell: 0, res:  0 },
    { level: 1, pos: 4,  cell: 1, res:  1 },
    { level: 0, pos: 4,  cell: 2, res:  1 },
    { level: 0, pos: 6,  cell: 3, res:  0 },
    { level: 0, pos: 8,  cell: 4, res:  0 },
    { level: 1, pos: 8,  cell: 2, res:  0 },
    { level: 2, pos: 8,  cell: 1, res:  0 },
    { level: 2, pos: 16, cell: 2, res: 1 },
    { level: 1, pos: 16, cell: 4, res: 0 },
    { level: 1, pos: 20, cell: 5, res: 1 },
    { level: 1, pos: 24, cell: 6, res: 0 },
    { level: 2, pos: 24, cell: 3, res: 0 },
    { level: 2, pos: 32, cell: 4, res: 0 },
    { level: 2, pos: 40, cell: 5, res: 0 },
    { level: 2, pos: 48, cell: 6, res: 0 },
    { level: 2, pos: 56, cell: 7, res: 0 },
  ]

  c.print()
  var ok = true
  c.traverse((state, i) => {
    ok = ok && diffObject(expect[i], state)
    ok = ok && i < expect.length
  })
  t.ok(ok, "path matches")
  t.end()
})

tape('edge of a level', (t) => {
  const c = createCascade(8, 3).set(31, 1)
  const expect = [
    { level: 2, pos: 0,  cell: 0, res:  0 },
    { level: 2, pos: 8,  cell: 1, res:  0 },
    { level: 2, pos: 16,  cell: 2, res:  0 },
    { level: 2, pos: 24,  cell: 3, res:  1 },
    { level: 1, pos: 24,  cell: 6, res:  0 },
    { level: 1, pos: 28,  cell: 7, res:  1 },
    { level: 1, pos: 32,  cell: 8, res:  -1 },
    { level: 2, pos: 32,  cell: 4, res:  0 },
    { level: 2, pos: 40,  cell: 5, res:  0 },
    { level: 2, pos: 48,  cell: 6, res:  0 },
    { level: 2, pos: 56,  cell: 7, res:  0 },
  ]

  c.print()
  var ok = true
  c.traverse((state, i) => {
    ok = ok && diffObject(expect[i], state)
    ok = ok && i < expect.length
  })
  t.ok(ok, "path matches")
  t.end()
})


function diffObject(expect, actual) {
  var ok = true
  if (!expect) {
    return false
  }
  const res = Object.keys(expect).map((k) => {
    ok = ok && expect[k] === actual[k]
    return [k, expect[k], actual[k]]
  })

  if (ok) {
    process.stdout.write(chalk.green('✓ '))
  } else {
    process.stdout.write(chalk.red('✖ '))
  }

  var aout = []

  var str = res.map((r, i) => {
    var match = r[1] === r[2]
    var prefix = `${r[0]}: `
    var alen = String(r[2]).length
    if (match) {
      aout.push((' ').repeat(prefix.length + String(r[1]).length + 1))
      return `${prefix}${chalk.green(r[1])}`
    } else {
      var extra = i === 0 ? 2 : 0
      var l = (' ').repeat(extra + prefix.length)
      aout.push(l + chalk.grey(r[1]))
      return `${prefix}${chalk.red(r[2])}`
    }
  }).join(', ')

  console.log(str)
  !ok && console.log(aout.join('  '))
  return ok
}

