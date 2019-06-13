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

bool voxel_mip_get(in vec3 pos, const in uint mip, out uint8_t palette_idx) {
  if (
    any(lessThan(pos, vec3(0.0))) ||
    any(greaterThanEqual(pos, vec3(dims)))
  ) {
    return false;
  }

  float r = 1.0 / float(1<<mip);
  vec3 d = floor(dims * r);
  vec3 p = floor(pos * r);

  uint idx = uint(p.x + p.y * d.x + p.z * d.x * d.y);

  switch (mip) {
    case 0: palette_idx = volumeMip0[idx]; break;
    case 1: palette_idx = volumeMip1[idx]; break;
    case 2: palette_idx = volumeMip2[idx]; break;
    case 3: palette_idx = volumeMip3[idx]; break;
    case 4: palette_idx = volumeMip4[idx]; break;
    case 5: palette_idx = volumeMip5[idx]; break;
    case 6: palette_idx = volumeMip6[idx]; break;
  }

  return palette_idx > uint8_t(0);
}
