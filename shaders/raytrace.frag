#version 330 core

in vec3 rayOrigin;

out vec4 outColor;

uniform sampler3D volume;
uniform uvec3 dims;
uniform vec3 eye;
uniform vec3 center;
uniform int showHeat;

#define ITERATIONS 256

float voxel(vec3 worldPos) {
  vec3 fdims = vec3(dims);
  vec3 hfdims = fdims * 0.5;

  vec3 pos = floor((hfdims + worldPos - center)) / fdims;
  return any(lessThan(pos, vec3(0.0))) || any(greaterThan(pos, vec3(1.0))) ? -1.0 : texture(volume, pos).r;
  // return texture(volume, pos).r;
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

// phong shading
vec3 shading( vec3 v, vec3 n, vec3 eye ) {
  vec3 final = vec3( 0.0 );

  // light 0
  {
    vec3 light_pos = eye + vec3(0.0, 0.0, 10.0);//vec3( 100.0, 110.0, 150.0 );
    vec3 light_color = vec3( 1.0 );
    vec3 vl = normalize( light_pos - v );
    float diffuse  = max( 0.0, dot( vl, n ) );
    final += light_color * diffuse;
  }

  // light 1
  {
    vec3 light_pos = -vec3( 100.0, 110.0, 120.0 );
    vec3 light_color = vec3( 1.0 );
    vec3 vl = normalize( light_pos - v );
    float diffuse  = max( 0.0, dot( vl, n ) );
    final += light_color * diffuse;
  }

  return final;
}

float march(in out vec3 pos, vec3 dir, out vec3 center, out float hit, out float iterations) {
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
  for (iterations = 0.0; iterations < ITERATIONS; iterations++ ) {
    if (hit > 0.0 || voxel( grid ) > 0.0) {
      hit = 1.0;
      continue;
    }
    iterations++;

    /*
      x < y && x < z :: step x
      x < y && x > z :: step z
      x > y && y < z :: step y
      x > y && y > z :: step z
    */
    mask = lessThanEqual(ratio.xyz, min(ratio.yzx, ratio.zxy));
    grid += ivec3(grid_step) * ivec3(mask);
    pos += grid_step * ivec3(mask);
    ratio += ratio_step * vec3(mask);
  }

  center = floor(pos) + vec3( 0.5 );

  // vec3 s = sign(dir);
  // vec3 diff = normalize(pos - (floor(pos) + vec3(0.5)));
  // vec3 m = vec3(greaterThan(diff.xyz, max(diff.yzx, diff.zxy)));

  // TODO: ensure we go the right direction into the voxel to get the center
  // normal = m * sign(diff);//normalize(pos - floor(pos) + vec3(0.5));
  // normal = vec3(mask);
  return dot( ratio - ratio_step, vec3(mask) ) * hit;
}

void main1() {
  vec3 pos = gl_FrontFacing ? rayOrigin : eye;
  vec3 eyeToPlane = gl_FrontFacing ? rayOrigin - eye : eye - rayOrigin;
  vec3 dir = normalize(eyeToPlane);
 
  vec3 diff = pos - center;
  vec3 adiff = abs(diff);

  vec3 mask = vec3(greaterThan(adiff.xyz, max(adiff.yzx, adiff.zxy)));

  vec3 color = mask;//((vec3(1.0) + diff) * mask) * 0.5;
  outColor = vec4(color, 1.0);
}

void main() {
  vec3 pos = gl_FrontFacing ? rayOrigin : eye;
  vec3 eyeToPlane = gl_FrontFacing ? rayOrigin - eye : eye - rayOrigin;
  vec3 dir = normalize(eyeToPlane);

  vec3 normal;
  vec3 voxelCenter;
  float hit, iterations;
  float depth = march(pos, dir, voxelCenter, hit, iterations);
  gl_FragDepth = hit < 0.0 ? 1.0 : -depth;

  vec3 color;

  vec3 fdims = vec3(dims);
  vec3 hfdims = fdims * 0.5;

  vec3 local = (hfdims + pos - center) / fdims;


  vec3 adiff = fract(local);
  vec3 mask = vec3(greaterThan(adiff.xyz, max(adiff.yzx, adiff.zxy)));

  color = vec3(depth/1.0);
  color = local;



  outColor = mix(vec4(color, 1.0), heat(iterations, ITERATIONS), showHeat);
}
