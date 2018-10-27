const tx = require('./tx')

module.exports = square

function square(ctx, mat, x, y, w, h) {
  const lineTo = ctx.lineTo.bind(ctx)
  const moveTo = ctx.moveTo.bind(ctx)
  tx(mat, x, y, moveTo)
  tx(mat, x, y + h, lineTo)
  tx(mat, x + w, y + h, lineTo)
  tx(mat, x + w, y, lineTo)
  tx(mat, x, y, lineTo)
}