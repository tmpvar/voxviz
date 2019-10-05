const test = require('ava')
const createTickBuffer = require('./tickbuffer')

test('createTickBuffer', t => {
  const tb = createTickBuffer(1, o => {
    o.abc = 123
    return o
  })

  t.truthy(tb)
  t.is(tb.buffer.length, 1)
  t.is(tb.buffer[0].abc, 123)
})

test('tick', t => {
  const tb = createTickBuffer(2, o => {
    o.abc = 123
  },
  (prev, current) => {
    current.abc = prev.abc + 1
    return current
  })

  tb.tick()
  t.is(tb.value(0).abc, 124)
  t.is(tb.value(-1).abc, 123)

  tb.tick()
  t.is(tb.value(0).abc, 125)
  t.is(tb.value(-1).abc, 124)

  tb.tick()
  t.is(tb.value(0).abc, 126)
  t.is(tb.value(-1).abc, 125)
})
