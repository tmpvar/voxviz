#include "../morton.glsl"

uvec2 binpack(uvec3 pos) {
  uvec3 word_pos = uvec3(pos) >> uvec3(0);
  uint word_idx = EncodeMorton3(word_pos);

  uvec3 bit_pos = uvec3(
    pos.x & 2,
    pos.y & 2,
    pos.z & 2
  );

  uint bit_idx = EncodeMorton3(bit_pos);

  return uvec2(
    word_idx,
    bit_idx
  );
}

uint64_t binpack_mask(uint bitpos) {
  return 1UL<<uint64_t(bitpos);
}
