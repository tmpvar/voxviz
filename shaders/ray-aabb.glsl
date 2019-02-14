
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
}bool test_ray_aabb(vec3 boxCenter, vec3 boxRadius, vec3 rayOrigin, vec3 rayDirection, vec3 invRayDirection) {
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

bool scratchHitAABox(vec3 boxCenter, vec3 boxRadius, vec3 rayOrigin, vec3 rayDirection, vec3 invRayDirection) {
  rayOrigin -= boxCenter;

  float tmin, tmax, tymin, tymax, tzmin, tzmax;
  vec3 bounds[2];
  bounds[0] = -boxRadius;
  bounds[1] = boxRadius;

  ivec3 sgn;
  sgn[0] = (invRayDirection.x < 0) ? 1 : 0;
  sgn[1] = (invRayDirection.y < 0) ? 1 : 0;
  sgn[2] = (invRayDirection.z < 0) ? 1 : 0;

  tmin = (bounds[sgn[0]].x - rayOrigin.x) * invRayDirection.x;
  tmax = (bounds[1 - sgn[0]].x - rayOrigin.x) * invRayDirection.x;
  tymin = (bounds[sgn[1]].y - rayOrigin.y) * invRayDirection.y;
  tymax = (bounds[1 - sgn[1]].y - rayOrigin.y) * invRayDirection.y;

  if ((tmin > tymax) || (tymin > tmax))  return false;
  if (tymin > tmin) tmin = tymin;
  if (tymax < tmax) tmax = tymax;

  tzmin = (bounds[sgn[2]].z - rayOrigin.z) * invRayDirection.z;
  tzmax = (bounds[1 - sgn[2]].z - rayOrigin.z) * invRayDirection.z;

  if ((tmin > tzmax) || (tzmin > tmax)) return false;
  if (tzmin > tmin) tmin = tzmin;
  if (tzmax < tmax) tmax = tzmax;

  float distance = (tmin > 0.0) ? tmin : tmax;
  return distance > 0.0;
}