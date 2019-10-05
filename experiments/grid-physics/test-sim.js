const test = require('ava')

const createSim = require('./sim')
const createBody = require('./body')
const createGrid = require('./grid')

test('createSim', t => {
  const grid = createGrid([10, 10])
  const sim = createSim(grid)
  t.truthy(sim)
})

test('no grid', t => {
  const sim = createSim()
  t.falsy(sim)
})

test('two bodies 1x1', t => {
  const grid = createGrid([100, 100])
  const sim = createSim(grid)

  const a = createBody([0.0, 0.0], [1, 1])
  const b = createBody([0.0, 0.5], [1, 1])

  sim.addBody(a)
  sim.addBody(b)

  const contacts = sim.tick(1)

  t.is(contacts[0].a, 1)
  t.is(contacts[0].b, 0)
  t.is(contacts[0].cell[0], 0.0)
  t.is(contacts[0].cell[1], 0.0)
})

test('collision response', t => {
  const grid = createGrid([10, 10])
  const sim = createSim(grid)

  const a = createBody([0.0, 0.0], [1, 1])
  const b = createBody([0.0, 0.5], [1, 1])

  sim.addBody(a)
  sim.addBody(b)
  var contacts

  contacts = sim.tick(1)
  sim.computeCollisionResponse(contacts)

  contacts = sim.tick(1)
  sim.computeCollisionResponse(contacts)

  t.is(sim.tick(1).length, 0)
})
