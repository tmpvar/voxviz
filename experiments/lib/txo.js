const { vec2 } = require('gl-matrix')

module.exports = txo

function txo(out, mat, vec) {
    vec2.transformMat3(out, vec, mat)
    return out
}
