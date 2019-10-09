const test = require('ava')
const { circleOBB } = require('./isect')

/*
CASES

1. 1x1 vs filled grid cell (no previous position)
2. 1x1 vs filled grid cell (has previous position)
3. NxM vs filled grid cell


*/


test('circleOBB: contact normal', t => {
  const ret = circleOBB(
    [5, 5],
    5,
    [2, 2]
  )
})
