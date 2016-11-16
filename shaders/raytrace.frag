#version 330 core

in vec3 rayOrigin;
in vec4 color;
out vec4 outColor;

uniform sampler3D volume;
uniform ivec3 dims;
uniform vec3 eye;

float getVoxel(vec3 worldPos, vec3 fdims) {
  vec3 pos = (fdims / 2.0 + worldPos)/ (fdims);

  if (any(greaterThan(pos, vec3(1.0)))) {
    return 0;
  }

  if (any(lessThan(pos, vec3(0)))) {
    return 0;
  }

  return texture(volume, pos).r;
}

void main() {
  vec3 fdims = vec3(dims);
  vec3 pos = rayOrigin;
  vec3 dir = normalize(rayOrigin - eye);

  ivec3 mapPos = ivec3(floor(pos + 0.));

  vec3 deltaDist = abs(vec3(length(dir)) / dir);

  ivec3 rayStep = ivec3(sign(dir));

  vec3 sideDist = (sign(dir) * (vec3(mapPos) - pos) + (sign(dir) * 0.5) + 0.5) * deltaDist;

  // bvec3 mask;

  vec3 uv = (fdims / 2.0 + pos)/ (fdims);

  bvec3 mask;
  for (int i=0; i<10000; i++) {
    if (getVoxel(mapPos, fdims) > 0.0) {
      outColor = vec4(uv, 1.0);
      break;
    }

    mask = lessThanEqual(sideDist.xyz, min(sideDist.yzx, sideDist.zxy));

    //All components of mask are false except for the corresponding largest component
    //of sideDist, which is the axis along which the ray should be incremented.

    sideDist += vec3(mask) * deltaDist;
    mapPos += ivec3(mask) * rayStep;

    if (any(greaterThanEqual(abs(mapPos), dims))) {
      discard;
      return;
    }
  }

  if (mask.x) {
    outColor = vec4(uv * 0.5, 1.0);
  } else if (mask.y) {
    outColor = vec4(uv * 1.0, 1.0);
  } else if (mask.z) {
    outColor = vec4(uv * 0.75, 1.0);
  }

}

