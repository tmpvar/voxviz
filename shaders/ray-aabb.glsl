
float min_component(vec3 a) {
  return min(a.x, min(a.y, a.z));
}

float max_component(vec3 a) {
  return max(a.x, max(a.y, a.z));
}

bool ray_aabb(vec3 p0, vec3 p1, vec3 rayOrigin, vec3 invRaydir) {
  vec3 t0 = (p0 - rayOrigin) * invRaydir;
  vec3 t1 = (p1 - rayOrigin) * invRaydir;
  vec3 tmin = min(t0, t1);
  vec3 tmax = max(t0, t1);
  return max_component(tmin) <= min_component(tmax);
}

bool test_ray_aabb(vec3 boxCenter, vec3 boxRadius, vec3 rayOrigin, vec3 rayDirection, vec3 invRayDirection) {
  rayOrigin -= boxCenter;
  vec3 distanceToPlane = (-boxRadius * sign(rayDirection) - rayOrigin) * invRayDirection;

# define TEST(U,V,W) (float(distanceToPlane.U >= 0.0) * float(abs(rayOrigin.V + rayDirection.V * distanceToPlane.U) < boxRadius.V) * float(abs(rayOrigin.W + rayDirection.W * distanceToPlane.U) < boxRadius.W))

  // If the ray is in the box or there is a hit along any axis, then there is a hit
  return bool(float(abs(rayOrigin.x) < boxRadius.x) *
    float(abs(rayOrigin.y) < boxRadius.y) *
    float(abs(rayOrigin.z) < boxRadius.z) +
    TEST(x, y, z) +
    TEST(y, z, x) +
    TEST(z, x, y));
# undef TEST
}

// Returns true if R intersects the oriented box. If there is an intersection, sets distance and normal based on the
// hit point. If no intersection, then distance and normal are undefined.
int indexOfMaxComponent(vec3 v) { return (v.x > v.y) ? ((v.x > v.z) ? 0 : 2) : (v.z > v.y) ? 2 : 1; }
float max3(float a, float b, float c) {
  return max(a, max(b, c));
}
float maxComponent(vec3 a) {
  return max3(a.x, a.y, a.z);
}

float min3(float a, float b, float c) {
  return min(a, min(b, c));
}

float minComponent(vec3 a) {
  return min3(a.x, a.y, a.z);
}


// Returns true if R intersects the oriented box. If there is an intersection, sets distance and normal based on the
// hit point. If no intersection, then distance and normal are undefined.
bool ailaWaldHitAABox(
  in const vec3 boxCenter,
  in const vec3 boxRadius,
  in const vec3 rayOrigin,
  in const vec3 rayDirection,
  in const vec3 invRayDirection,
  out float found_distance,
  out vec3 found_normal
) {
  vec3 origin = rayOrigin - boxCenter;

  vec3 t_min = (-boxRadius - origin) * invRayDirection;
  vec3 t_max = (boxRadius - origin) * invRayDirection;
  float t0 = maxComponent(min(t_min, t_max));
  float t1 = minComponent(max(t_min, t_max));

  // Compute the intersection distance
  found_distance = (t0 > 0.0) ? t0 : t1;

  //vec3 V = boxCenter - (rayOrigin + rayDirection * found_distance) * (1.0 / boxRadius);
  //vec3 mask = vec3(lessThan(vec3(lessThan(V.xyz, V.yzx)), V.zxy));// step(V.xyz, V.yzx) * step(V.xyz, V.zxy);
  //found_normal = mask;// * (all(lessThan(abs(origin), boxRadius)) ? -1.0 : 1.0);


  return (t0 <= t1) && (found_distance > 0.0) && !isinf(found_distance);
}
