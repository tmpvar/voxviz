layout (std430, binding=1) buffer volumeSlabMip0 {
  uint8_t volumeMip0[];
};

layout (std430, binding=2) buffer volumeSlabMip1 {
  uint8_t volumeMip1[];
};

layout (std430, binding=3) buffer volumeSlabMip2 {
  uint8_t volumeMip2[];
};

layout (std430, binding=4) buffer volumeSlabMip3 {
  uint8_t volumeMip3[];
};

layout (std430, binding=5) buffer volumeSlabMip4 {
  uint8_t volumeMip4[];
};

layout (std430, binding=6) buffer volumeSlabMip5 {
  uint8_t volumeMip5[];
};

layout (std430, binding=7) buffer volumeSlabMip6 {
  uint8_t volumeMip6[];
};

uint voxel_get_idx(vec3 pos) {
  vec3 p = floor(pos);
  return uint(
    p.x +
    p.y * dims.x +
    p.z * dims.x * dims.y
  );
}

bool voxel_get(vec3 pos, out uint8_t palette_idx) {
  vec3 p = floor(pos);
  if (
    any(lessThan(p, vec3(0))) ||
    any(greaterThanEqual(p, vec3(dims)))
  ) {
    return false;
  }

  uint idx = uint(
    p.x +
    p.y * dims.x +
    p.z * dims.x * dims.y
  );

  palette_idx = volumeMip0[idx];

  return palette_idx > uint8_t(0);
}

bool voxel_get_mip1(vec3 pos, out uint8_t palette_idx) {
  // vec3 d = dims / 2.0;
  // pos = pos / 2.0;
  vec3 d = floor(dims / 2.0);
  vec3 p = floor(pos / 2.0);
  if (
    any(lessThan(p, vec3(0.0))) ||
    any(greaterThanEqual(p, vec3(d)))
  ) {
    return false;
  }

  uint idx = uint(
    (
      p.x +
      p.y * d.x +
      p.z * d.x * d.y
    )
  );

  palette_idx = volumeMip1[idx];

  return palette_idx > uint8_t(0);
}

bool voxel_get_mip2(vec3 pos, out uint8_t palette_idx) {
  // vec3 d = dims / 2.0;
  // pos = pos / 2.0;
  vec3 d = floor(dims / 4.0);
  vec3 p = floor(pos / 4.0);
  if (
    any(lessThan(p, vec3(0.0))) ||
    any(greaterThanEqual(p, vec3(d)))
  ) {
    return false;
  }

  uint idx = uint(
    (
      p.x +
      p.y * d.x +
      p.z * d.x * d.y
    )
  );

  palette_idx = volumeMip2[idx];

  return palette_idx > uint8_t(0);
}

bool voxel_get_mip3(vec3 pos, out uint8_t palette_idx) {
  vec3 d = floor(dims / 8.0);
  vec3 p = floor(pos / 8.0);
  if (any(lessThan(p, ivec3(0))) || any(greaterThanEqual(p, ivec3(d)))) {
    return false;
  }

  uint idx = uint(
    p.x +
    p.y * d.x +
    p.z * d.x * d.y
  );

  palette_idx = volumeMip3[idx];
  return palette_idx > uint8_t(0);
}

bool voxel_get_mip4(vec3 pos, out uint8_t palette_idx) {
  vec3 d = floor(dims / 16.0);
  vec3 p = floor(pos / 16.0);
  if (any(lessThan(p, ivec3(0))) || any(greaterThanEqual(p, ivec3(d)))) {
    return false;
  }

  uint idx = uint(
    p.x +
    p.y * d.x +
    p.z * d.x * d.y
  );

  palette_idx = volumeMip4[idx];

  return palette_idx > uint8_t(0);
}

bool voxel_get_mip5(vec3 pos, out uint8_t palette_idx) {
  vec3 d = floor(dims / 32.0);
  vec3 p = floor(pos / 32.0);
  if (any(lessThan(p, ivec3(0))) || any(greaterThanEqual(p, ivec3(d)))) {
    return false;
  }

  uint idx = uint(
    p.x +
    p.y * d.x +
    p.z * d.x * d.y
  );

  palette_idx = volumeMip5[idx];

  return palette_idx > uint8_t(0);
}

bool voxel_get_mip6(vec3 pos, out uint8_t palette_idx) {
  vec3 d = floor(dims / 64.0);
  vec3 p = floor(pos / 64.0);
  if (any(lessThan(p, ivec3(0))) || any(greaterThanEqual(p, ivec3(d)))) {
    return false;
  }

  uint idx = uint(
    p.x +
    p.y * d.x +
    p.z * d.x * d.y
  );

  palette_idx = volumeMip5[idx];

  return palette_idx > uint8_t(0);
}

bool voxel_mip(in vec3 pos, const in uint mip, out uint8_t palette_idx) {
  vec3 gridPos = pos;// * dims;
  if (mip == 0) {
    return voxel_get(gridPos, palette_idx);
  }

  if (mip == 1) {
    return voxel_get_mip1(gridPos, palette_idx);
  }

  if (mip == 2) {
    return voxel_get_mip2(gridPos, palette_idx);
  }

  if (mip == 3) {
    return voxel_get_mip3(gridPos, palette_idx);
  }

  if (mip == 4) {
    return voxel_get_mip4(gridPos, palette_idx);
  }

  if (mip == 5) {
    return voxel_get_mip5(gridPos, palette_idx);
  }

  if (mip == 6) {
    return voxel_get_mip6(gridPos, palette_idx);
  }

  return false;
}
