#version 450 core
#extension GL_NV_gpu_shader5: require
#extension GL_ARB_bindless_texture : require

in vec3 rayOrigin;
in vec3 center;
flat in float *volumePointer;

layout(location = 0) out vec4 outColor;

uniform sampler3D volume;
uniform uvec3 dims;
uniform vec3 eye;
uniform float maxDistance;

#define ITERATIONS 60

float voxel(vec3 worldPos) {
  vec3 fdims = vec3(dims);
  vec3 hfdims = fdims * 0.5;

  uvec3 pos = uvec3(round((hfdims + worldPos - center)));
  uint idx = (pos.x + pos.y * dims.x + pos.z * dims.x * dims.y);
  bool oob = any(lessThan(pos, vec3(0.0))) || any(greaterThanEqual(pos, dims));
  return oob ? -1.0 : float(volumePointer[idx]);
}

float march(in vec3 pos, in vec3 dir, out float hit) {
  dir = normalize(dir);
  // grid space
  vec3 origin = pos;
  vec3 grid = floor(pos);
  vec3 grid_step = sign( dir );
  vec3 corner = max( grid_step, vec3( 0.0 ) );
  bvec3 mask;

  // ray space
  vec3 inv = vec3( 1.0 ) / dir;
  vec3 ratio = ( grid + corner - pos ) * inv;
  vec3 ratio_step = grid_step * inv;

  // dda
  hit = -1.0;

  for (float i = 0.0; i < ITERATIONS; i++ ) {
    if (hit > 0.0 || voxel( grid ) > 0.0) {
      hit = 1.0;
      continue;
    }

    mask = lessThanEqual(ratio.xyz, min(ratio.yzx, ratio.zxy));
    grid += ivec3(grid_step) * ivec3(mask);
	pos += grid_step * ivec3(mask);
    ratio += ratio_step * vec3(mask);
  }

  return distance(eye, pos);
}

void main() {
  vec3 pos = gl_FrontFacing ? rayOrigin : eye;
  vec3 eyeToPlane = gl_FrontFacing ? rayOrigin - eye : eye - rayOrigin;
  vec3 dir = normalize(eyeToPlane);

  vec3 normal;
  vec3 voxelCenter;
  float hit;

  float depth = march(pos, dir, hit);
  gl_FragDepth = hit < 0.0 ? 1.0 : depth / maxDistance;
  outColor = vec4(normal, 1.0);
}
