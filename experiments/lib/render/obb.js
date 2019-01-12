module.exports = obb

function obb(ctx, points) {
  const last = points[points.length-1]
  ctx.moveTo(last[0], last[1])
  points.forEach((p) => {
    ctx.lineTo(p[0], p[1])
  })
}
