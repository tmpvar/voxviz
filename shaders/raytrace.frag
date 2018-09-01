#version 450 core
#extension GL_NV_gpu_shader5: require
#extension GL_ARB_bindless_texture : require
//#extension GL_EXT_shader_image_load_store : require

in vec3 rayOrigin;
in vec3 localRayOrigin;
in vec3 center;
flat in float *volumePointer;
//flat in layout(bindless_sampler) sampler3D volumeSampler;

layout(location = 0) out vec4 outColor;
layout(location = 1) out vec3 outPosition;

// main binds
uniform uvec3 dims;
uniform float debug;
uniform mat4 MVP;
uniform vec3 eye;
uniform int showHeat;
uniform float maxDistance;
#define ITERATIONS 60

float voxel(vec3 brickPos) {
  uvec3 pos = uvec3(brickPos);
  uint idx = (pos.x + pos.y * dims.x + pos.z * dims.x * dims.y);
  bool oob = any(lessThan(pos, vec3(0.0))) || any(greaterThanEqual(pos, dims));
  return oob ? -1.0 : float(volumePointer[idx]);
}

vec3 hsv2rgb(vec3 c) {
  vec4 K = vec4(1.0, 0.6666666666, 0.33333333333, 3.0);
  vec3 p = abs(fract(c.xxx + K.xyz) * 6.0 - K.www);
  return c.z * mix(K.xxx, clamp(p - K.xxx, 0.0, 1.0), c.y);
}

vec4 heat(float amount, float total) {
  float p = (amount / total);
  vec3 hsv = vec3(0.66 - p, 1.0, .75);
  return vec4(hsv2rgb(hsv), 1.0);
}

float march(in out vec3 pos, vec3 dir, out vec3 normal, out float iterations) {
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
  float hit = -1.0;
  iterations = 0.0;
  for (float i = 0.0; i < ITERATIONS; i++ ) {
    if (hit > 0.0 || voxel( grid ) > 0.0) {
      hit = 1.0;
	  break;
    }
    iterations++;

    mask = lessThanEqual(ratio.xyz, min(ratio.yzx, ratio.zxy));
    grid += ivec3(grid_step) * ivec3(mask);
    pos += grid_step * ivec3(mask);
    ratio += ratio_step * vec3(mask);
  }
  
  vec3 d = abs(vec3((grid) + 0.5) - pos);

  normal = iterations == 0.0 ? vec3(greaterThan(d.xyz, max(d.yzx, d.zxy))) : vec3(mask);
  return hit;
}

void main() {
  vec3 origin = gl_FrontFacing ? rayOrigin : eye;
  vec3 pos = gl_FrontFacing ? rayOrigin : eye;
  vec3 eyeToPlane = gl_FrontFacing ? rayOrigin - eye : eye - rayOrigin;
  vec3 dir = normalize(eyeToPlane);
  vec3 normal;
  vec3 voxelCenter;
  float iterations;

  vec3 brickPos = localRayOrigin;
  float hit = march(brickPos, dir, normal, iterations);
  
  vec3 worldPos = origin + brickPos;

  gl_FragDepth = hit < 0.0 ? 1.0 : distance(worldPos, eye) / maxDistance;
  outColor = hit < 0.0 ? vec4(0.0) : vec4(normal, 1.0);//vec4(normal, 1.0);
  outPosition = worldPos / maxDistance;
}

void main1() {
	outColor = vec4(1.0);
}
