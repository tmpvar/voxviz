#ifndef SPLATS_UTIL
#define SPLATS_UTIL
  uint read(vec3 p, uint mipLevel) {
    if (any(lessThan(p, vec3(0.0))) || any(greaterThan(p, volumeSlabDims))) {
        return 0;
    }
    uint8_t nop;
    voxel_mip_get(round(p), mipLevel, nop);
    return nop < uint8_t(255) ? 0 : 1;
  }

  bool visible(vec3 p, uint mipLevel) {
    uint a = 1;
    float off = float(1<<mipLevel);
    a &= read(p + vec3(off, 0.0, 0.0), mipLevel);
    a &= read(p + vec3(-off, 0.0, 0.0), mipLevel);

    a &= read(p + vec3(0.0, off, 0.0), mipLevel);
    a &= read(p + vec3(0.0, -off, 0.0), mipLevel);

    a &= read(p + vec3(0.0, 0.0, off), mipLevel);
    a &= read(p + vec3(0.0, 0.0, -off), mipLevel);

    a &= read(p + vec3(0.0, off, off), mipLevel);
    a &= read(p + vec3(0.0, off, -off), mipLevel);
    a &= read(p + vec3(0.0, -off, off), mipLevel);
    a &= read(p + vec3(0.0, -off, -off), mipLevel);

    a &= read(p + vec3(off, 0.0, off), mipLevel);
    a &= read(p + vec3(off, 0.0, -off), mipLevel);
    a &= read(p + vec3(-off, 0.0, off), mipLevel);
    a &= read(p + vec3(-off, 0.0, -off), mipLevel);

    a &= read(p + vec3(off, off, 0.0), mipLevel);
    a &= read(p + vec3(off, -off, 0.0), mipLevel);
    a &= read(p + vec3(-off, off, 0.0), mipLevel);
    a &= read(p + vec3(-off, -off, 0.0), mipLevel);


    a &= read(p + vec3(off, off, off), mipLevel);
    a &= read(p + vec3(off, off, -off), mipLevel);
    a &= read(p + vec3(off, -off, off), mipLevel);
    a &= read(p + vec3(off, -off, -off), mipLevel);
    a &= read(p + vec3(-off, off, off), mipLevel);
    a &= read(p + vec3(-off, off, -off), mipLevel);
    a &= read(p + vec3(-off, -off, off), mipLevel);
    a &= read(p + vec3(-off, -off, -off), mipLevel);


    return a == 0;
  }
#endif
