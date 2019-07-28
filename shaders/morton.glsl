#ifndef _MORTON
#define _MORTON
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
#endif
