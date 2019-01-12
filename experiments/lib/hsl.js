module.exports = hsl

function hsl(p, a) {
  return `hsla(${p*360}, 100%, 65%, ${a||1})`
}
