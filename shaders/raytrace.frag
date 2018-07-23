#version 330 core

in vec3 rayOrigin;
in vec4 vertPosition;

layout(location = 0) out vec4 outColor;

uniform sampler3D volume;
uniform sampler2D shadowMap;

uniform uvec3 dims;
uniform vec3 eye;
uniform vec3 center;
uniform int showHeat;
uniform float debug;
uniform float maxDistance;
uniform mat4 depthBiasMVP;

#define ITERATIONS 768

float voxel(vec3 worldPos) {
  vec3 fdims = vec3(dims);
  vec3 hfdims = fdims * 0.5;

  vec3 pos = round((hfdims + worldPos - center)) / fdims;
  return any(lessThan(pos, vec3(0.0))) || any(greaterThan(pos, vec3(1.0))) ? -1.0 : texture(volume, pos).r;
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

float march(in out vec3 pos, vec3 dir, out vec3 center, out vec3 normal, out float hit, out float iterations) {
  dir = normalize(dir);
  // grid space
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
  iterations = 0.0;
  for (float i = 0.0; i < ITERATIONS; i++ ) {
    if (hit > 0.0 || voxel( grid ) > 0.0) {
      hit = 1.0;
      continue;
    }
    iterations++;

    mask = lessThanEqual(ratio.xyz, min(ratio.yzx, ratio.zxy));
    grid += ivec3(grid_step) * ivec3(mask);
    pos += grid_step * ivec3(mask);
    ratio += ratio_step * vec3(mask);
  }

  center = floor(pos) + vec3( 0.5 );

  vec3 d = abs(center - pos);
  normal = iterations == 0.0 ? vec3(greaterThan(d.xyz, max(d.yzx, d.zxy))) : vec3(mask);
  return distance(eye, pos) / maxDistance;
}

void main() {
  vec3 origin = gl_FrontFacing ? rayOrigin : eye;
  vec3 pos = gl_FrontFacing ? rayOrigin : eye;
  vec3 eyeToPlane = gl_FrontFacing ? rayOrigin - eye : eye - rayOrigin;
  vec3 dir = normalize(eyeToPlane);

  vec3 normal;
  vec3 voxelCenter;
  float hit, iterations;

  float depth = march(pos, dir, voxelCenter, normal, hit, iterations);
  gl_FragDepth = hit < 0.0 ? 1.0 : depth;

  vec3 color;
  color = normal;
  color = mix(color, vec3(1.0, 0.0, 0.0), debug);

  vec4 shadowCoord = depthBiasMVP * vec4(origin + dir * depth, 1.0);
  vec2 shadowUV = shadowCoord.xy / shadowCoord.w;
  
  float visibility = 1.0;
  if (shadowUV.y >= 0.0 && shadowUV.x >= 0.0 && shadowUV.y <= 1.0 && shadowUV.x <= 1.0) {
    visibility = texture(shadowMap, shadowUV).r < (shadowCoord.z / shadowCoord.w) ? 0.5 : 1.0;
  }

  //color = vec3((1.0 + shadowUV.yx) * 0.5, texture( shadowMap, shadowUV ).x / maxDistance);//
  color = color * visibility;
  outColor = mix(vec4(color, 1.0), heat(iterations, ITERATIONS), showHeat);
}
