struct RayTermination {
  vec4 position;
  vec4 normal;
  vec4 color;
  vec4 rayDir;
};

float mincomp(vec3 v) {
  return min(v.x, min(v.y, v.z));
}
