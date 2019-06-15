#include "mips.glsl"

layout (std430) buffer lightSlabBuffer {
  vec4 lightSlab[];
};
uniform vec3 lightSlabDims;

bool light_mip_get(in vec3 pos, const in uint mip, out vec4 color) {
  uvec3 p = uvec3(
    uint(pos.x) >> mip,
    uint(pos.y) >> mip,
    uint(pos.z) >> mip
  );

  uvec3 d = uvec3(
    uint(lightSlabDims.x) >> mip,
    uint(lightSlabDims.y) >> mip,
    uint(lightSlabDims.z) >> mip
  );

  if (any(greaterThanEqual(p, d))) {
    return false;
  }

  float slots = lightSlabDims.x * lightSlabDims.y * lightSlabDims.z;
  vec2 lod = lod_defs[mip];
  uint offset = uint(floor(slots * lod.x));

  uint idx = offset + uint(
    p.x + p.y * d.x + p.z * d.x * d.y
  );

  color = lightSlab[idx];

  return color.a > 0.0;
}

void light_mip_set(in vec3 pos, const in uint mip, const in vec4 color) {
  uvec3 p = uvec3(
    uint(pos.x) >> mip,
    uint(pos.y) >> mip,
    uint(pos.z) >> mip
  );

  uvec3 d = uvec3(
    uint(lightSlabDims.x) >> mip,
    uint(lightSlabDims.y) >> mip,
    uint(lightSlabDims.z) >> mip
  );

  if (any(greaterThanEqual(p, d))) {
    return;
  }

  float slots = lightSlabDims.x * lightSlabDims.y * lightSlabDims.z;
  vec2 lod = lod_defs[mip];
  uint offset = uint(floor(slots * lod.x));

  uint idx = offset + uint(
    p.x + p.y * d.x + p.z * d.x * d.y
  );

  lightSlab[idx] = color;
}
