layout (std430, binding=1) buffer volumeSlabBuffer {
  uint8_t volumeSlab[];
};

const vec2 lod_defs[8] = vec2[8](
    vec2(0, 1),
    vec2(1, 0.12499999999),
    vec2(1.12499999999, 0.0156249999975),
    vec2(1.1406249999875, 0.00195312499953125),
    vec2(1.1425781249870313, 0.000244140624921875),
    vec2(1.142822265611953, 0.000030517578112792968),
    vec2(1.142852783190066, 0.000003814697263793945),
    vec2(1.1428565978873297, 4.7683715793609617e-7)
);

bool voxel_mip_get(in vec3 pos, const in uint mip, out uint8_t palette_idx) {
  uvec3 p = uvec3(
    uint(pos.x) >> mip,
    uint(pos.y) >> mip,
    uint(pos.z) >> mip
  );

  uvec3 d = uvec3(
    uint(dims.x) >> mip,
    uint(dims.y) >> mip,
    uint(dims.z) >> mip
  );

  if (any(greaterThanEqual(p, d))) {
    return false;
  }

  float slots = dims.x * dims.y * dims.z;
  vec2 lod = lod_defs[mip];
  uint offset = uint(floor(slots * lod.x));

  //uvec3 upos = uvec3(pos)
  // vec3 d = dims * lod.y;
  // vec3 p = pos * lod.y;
  uint idx = offset + uint(
    p.x + p.y * d.x + p.z * d.x * d.y
  );

  // uint idx = offset + uint(
  //   (
  //     pos.x +
  //     pos.y * dims.x +
  //     pos.z * dims.x * dims.y
  //   ) * lod.y
  // );

  palette_idx = volumeSlab[idx];

  return palette_idx > uint8_t(0);
}
