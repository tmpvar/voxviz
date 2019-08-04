layout (std430) buffer volumeSlabBuffer {
  uint8_t volumeSlab[];
};

uniform vec3 volumeSlabDims;

#include "mips.glsl"
#include "morton.glsl"

bool voxel_mip_get(in vec3 pos, const in uint mip, out uint8_t palette_idx) {
  uvec3 p = uvec3(
    uint(pos.x) >> mip,
    uint(pos.y) >> mip,
    uint(pos.z) >> mip
  );

  uvec3 d = uvec3(
    uint(volumeSlabDims.x) >> mip,
    uint(volumeSlabDims.y) >> mip,
    uint(volumeSlabDims.z) >> mip
  );

  if (any(greaterThanEqual(p, d))) {
    return false;
  }

  float slots = volumeSlabDims.x * volumeSlabDims.y * volumeSlabDims.z;
  vec2 lod = lod_defs[mip];
  uint offset = uint(floor(slots * lod.x));

  uint idx = offset + uint(
    p.x + p.y * d.x + p.z * d.x * d.y
  );
  // if (mip > 0) {
  //   idx = offset + EncodeMorton3(p);
  // }

  palette_idx = volumeSlab[idx];

  return palette_idx > uint8_t(0);
}
