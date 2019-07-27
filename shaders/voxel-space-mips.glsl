layout (std430) buffer volumeSlabBuffer {
  uint8_t volumeSlab[];
};

uniform vec3 volumeSlabDims;

#include "mips.glsl"

// "Insert" two 0 bits after each of the 10 low bits of x
uint32_t Part1By2(uint32_t x)
{
  x &= 0x000003ff;                  // x = ---- ---- ---- ---- ---- --98 7654 3210
  x = (x ^ (x << 16)) & 0xff0000ff; // x = ---- --98 ---- ---- ---- ---- 7654 3210
  x = (x ^ (x <<  8)) & 0x0300f00f; // x = ---- --98 ---- ---- 7654 ---- ---- 3210
  x = (x ^ (x <<  4)) & 0x030c30c3; // x = ---- --98 ---- 76-- --54 ---- 32-- --10
  x = (x ^ (x <<  2)) & 0x09249249; // x = ---- 9--8 --7- -6-- 5--4 --3- -2-- 1--0
  return x;
}


uint32_t EncodeMorton3(uvec3 p)
{
  return ((Part1By2(p.z) << 2) + (Part1By2(p.y) << 1) + Part1By2(p.x));
}

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
  if (mip > 0) {
    idx = offset + EncodeMorton3(p);
  }

  palette_idx = volumeSlab[idx];

  return palette_idx > uint8_t(0);
}
