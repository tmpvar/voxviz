const test = require('ava')
const createGrid = require('./grid')

test('createGrid', t => {
  const grid = createGrid([100, 10])
  t.is(grid.dims[0], 100)
  t.is(grid.dims[1], 10)

  t.is(grid.data.shape[0], 100)
  t.is(grid.data.shape[1], 10)

  t.is(grid.time, 1)
})

test('tick', t => {
  const grid = createGrid([1, 1])
  t.is(grid.time, 1)
  grid.tick()
  t.is(grid.time, 2)
})

test('set - no overlap', t => {
  const grid = createGrid([10, 10])
  grid.set([5, 5], 1)
  t.is(grid.get([5, 5]).time, 1)
  t.is(grid.get([5, 5]).object, 1)
})

test('set - overlap', t => {
  const grid = createGrid([1, 1])
  grid.set([0, 0], 1337)
  const ret = grid.set([0, 0], 1338)
  t.is(ret, 1337)
  t.is(grid.get([0, 0]).time, 1)
  t.is(grid.get([0, 0]).object, 1337)
})

test('set - no overlap due to time', t => {
  const grid = createGrid([1, 1])
  t.is(grid.set([0, 0], 1337), false)
  grid.tick()
  t.is(grid.set([0, 0], 1338), false)
  t.is(grid.get([0, 0]).time, 2)
  t.is(grid.get([0, 0]).object, 1338)

  grid.time = 500
  t.is(grid.set([0, 0], 1339), false)
  t.is(grid.get([0, 0]).time, 500)
  t.is(grid.get([0, 0]).object, 1339)
})

test('set - no overlap due to same object', t => {
  const grid = createGrid([1, 1])
  grid.set([0, 0], 1337)
  grid.tick()
  t.is(grid.set([0, 0], 1337), false)
})
