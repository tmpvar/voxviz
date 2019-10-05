const test = require('ava')

const createBody = require('./body')
const grid = require('./grid')

test('createBody', t => {
  t.truthy(createBody([0, 0], [2, 2]))
})
