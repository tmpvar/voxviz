#version 330 core

in vec3 rayOrigin;

out vec4 outColor;

uniform sampler3D volume;
uniform ivec3 dims;
uniform vec3 eye;
uniform vec3 center;
uniform int showHeat;

float getVoxel(vec3 worldPos, vec3 fdims, vec3 hfdims) {
  vec3 pos = (hfdims + worldPos - center) / (fdims);
  return texture(volume, pos).r;
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

void main() {
  vec3 fdims = vec3(dims);
  vec3 hfdims = fdims / 2.0;
  vec3 pos = rayOrigin;
  vec3 dir = normalize(rayOrigin - eye);

  ivec3 mapPos = gl_FrontFacing ? ivec3(floor(pos)) : ivec3(floor(eye));

  vec3 deltaDist = abs(vec3(length(dir)) / dir);

  ivec3 rayStep = ivec3(sign(dir));

  vec3 sideDist = (sign(dir) * (vec3(mapPos) - pos) + (sign(dir) * 0.5) + 0.5) * deltaDist;
  int i;
  bool miss = true;
  for (i=0; i<400; i++) {
    bvec3 mask = lessThanEqual(sideDist.xyz, min(sideDist.yzx, sideDist.zxy));
    if (getVoxel(mapPos, fdims, hfdims) > 0.0) {

      outColor = vec4((hfdims + mapPos) / (fdims), 1.0);
      
      if (mask.x) {
        outColor = outColor * 0.5;
      } else if (mask.z) {
        outColor = outColor * 0.75;
      }
      miss = false;
      break;
    }

    sideDist += vec3(mask) * deltaDist;
    mapPos += ivec3(mask) * rayStep;

    if (any(lessThan(mapPos, (center-hfdims))) || any(greaterThan(mapPos, (center + hfdims)))) {
      break;
    }
  }

  if (showHeat == 1) {
    outColor = heat(i, 300);
    return;
  }

  // putting this inside of the if above causes the computer to hang
  // this works a bit better, but is probably slower?
  if (miss) {
    discard;
  }
}
