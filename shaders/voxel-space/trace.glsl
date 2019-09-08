float traceMine(vec3 pos, vec3 light_pos) {
  vec3 dir = normalize(light_pos - pos);
  float dist = 0.0;
  float aperture = tan(3.1459 / 2.0) * 40;
  float diam = dist * aperture;
  vec3 samplePos = dir * dist + pos;
  float maxDist = 128;
  float occ = 0.0;
  float falloff = 1.0/maxDist;
  while (dist < maxDist) {
    if (oob(samplePos)) {
      return 0.0;
    }

    if (occ >= 1) {
      break;
    }

    float d = distance(samplePos, light_pos);
    if (d < diam) {
      break;
    }

    if (d > maxDist) {
      return 1.0 - d/maxDist;
    }

    float mip = max(log2(diam/2), 0);

    float mipSample = 0.0;
    mipSample += .34 * read(samplePos, diam/4.0);
    mipSample += 0.52 * read(samplePos, 1 + 0.95 * diam);
    mipSample += 0.48 * read(samplePos, 5.5 *  diam);
    occ += (1.0 - occ) * mipSample * 0.45;// / (2.0 + 1.9 * diam);
    dist += max(diam, VOXEL_SIZE);
    diam = max(1, 2.0 * aperture * dist);
    samplePos = dir * dist + pos;
    occ+=0.0001;
    //occ += 1/diam * 0.001;//falloff / 20;
  }

  return (1.0 - occ) * 8.0;
  return 1.0 - pow(smoothstep(0, 1, occ * 0.5), 1.0 / 1.1);
}
