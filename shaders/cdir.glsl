vec3 sphereRandom(vec2 r) {
    float cosPhi = r.x * 2.0 - 1.0;
    float sinPhi = sqrt(1 - (cosPhi * cosPhi));
    float theta = r.y * 2.0 * 3.1415927;
    return vec3(sinPhi * cos(theta), sinPhi * cos(theta), cosPhi);
}

vec3 hemisphereRandom(vec2 r) {
    vec3 s = sphereRandom(r);
    return vec3(s.x, s.y, s.z);
}

vec3 sampleBlueNoise(in uint seed) {
  vec2 bn = blueNoise[(seed * time / 10000) % (4096 * 64)].xy;
  return sphereRandom(bn);
}

vec3 cdir(in uint seed, in vec3 normal) {
  // uint slice = (time % 64) * 4096;

  vec2 bn = 2.0 * blueNoise[(seed * time / 10000) % (4096 * 64)].xy - 1.0;
  vec3 d = hemisphereRandom(bn);

  if (normal.x == 1.0) {
    d.x = abs(d.x);
  }

  if (normal.y == 1.0) {
    d.y = abs(d.y);
  }

  if (normal.z == 1.0) {
    d.z = abs(d.z);
  }

  if (normal.x == -1.0) {
    d.x = -abs(d.x);
  }

  if (normal.y == -1.0) {
    d.y = -abs(d.y);
  }

  if (normal.z == -1.0) {
    d.z = -abs(d.z);
  }

  return d;


  float u = bn.x;
  float v = bn.y;

	// // method 3 by fizzer: http://www.amietia.com/lambertnotangent.html
  float a = 6.2831853 * v;
  //u = 2.0*u - 1.0;
  return normalize(
    normal + vec3(sqrt(1.0-u*u) * vec2(cos(a), sin(a)), u)
  );
}
