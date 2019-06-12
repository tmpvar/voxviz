const size = Math.pow(128, 3);
const levels = 8;
const dims = 3;

var a = 0;
var pos = 0
var v = 1
var parts = []

for (var i=0; i<levels; i++) {
  parts.push(`  vec2(${pos}, ${v})`)

  console.error("lod: %s (%s -> %s) %s slots", i, size*pos, size*v+size*pos, size*v)

  pos += v
  v *= 0.125

}
console.error()
console.error()

console.log('// precomputed voxel lod buffer position and size ratios')
console.log('// vec2(start, length)')
console.log('const vec2 lod_defs[%s] = [', levels)
console.log('  ' + parts.join(',\n  '))
console.log('];')
