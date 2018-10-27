const { vec2 } = require('gl-matrix')
const v2tmp = vec2.create()

module.exports = tx

function tx(mat, x, y, fn) {
    vec2.set(v2tmp, x, y)
    vec2.transformMat3(v2tmp, v2tmp, mat)
    fn(v2tmp[0], v2tmp[1])
}
