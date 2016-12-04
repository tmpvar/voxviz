#ifndef __ORBIT_CAMERA__
#define __ORBIT_CAMERA__

#include <glm/glm.hpp>
#include <glm/gtx/quaternion.hpp>
#include <glm/gtc/matrix_transform.hpp>

struct {
  glm::quat rotation;
  glm::vec3 center, v3scratch;
  float distance;
  glm::mat4 scratch0, scratch1;
} orbit_camera;

static void orbit_camera_lookat(const glm::vec3 eye, const glm::vec3 center, const glm::vec3 up) {
  orbit_camera.rotation = glm::normalize(glm::quat_cast(glm::lookAt(eye, center, up)));
  orbit_camera.center = center;
  orbit_camera.distance = glm::distance(eye, center);
}

static void orbit_camera_init(const glm::vec3 eye, const glm::vec3 center, const glm::vec3 up) {
  orbit_camera_lookat(eye, center, up);
}

static glm::quat quat_from_vec3(const glm::vec3 vec) {
  float x = vec[0];
  float y = vec[1];
  float z = vec[2];
  float s = x*x + y*y;
  if (s > 1.0) {
    s = 1.0;
  }

  if (z == 0.0) {
    z = sqrtf(1.0 - s);
  }

  return glm::quat(-x, y, z, 0.0);
}

static void orbit_camera_rotate(const float sx, const float sy, const float ex, const float ey) {
  const glm::vec3 vs = glm::vec3(sy, 0.0, sx);
  const glm::vec3 ve = glm::vec3(ey, 0.0, ex);

  glm::quat s = glm::quat(ve);
  glm::quat e = glm::conjugate(glm::quat(vs));

  s = s * e;

  if(glm::length(s) < 1e-6) {
    printf("MISS %f\n", glm::length(s));
    return;
  }

  orbit_camera.rotation = glm::normalize(s * orbit_camera.rotation);
}
/*
static glm::vec3 orbit_camera_unproject(const glm::vec3 vec, const vec4 viewport, const glm::mat4 inv) {
  float viewX = viewport[0];
  float viewY = viewport[1];
  float viewWidth = viewport[2];
  float viewHeight = viewport[3];

  float x = vec[0];
  float y = vec[1];
  float z = vec[2];

  x = x - viewX;
  y = viewHeight - y - 1;
  y = y - viewY;

  glm::vec3 r = glm::vec3(
    (2 * x) / viewWidth - 1,
    (2 * y) / viewHeight - 1,
    2 * z - 1
  );

  return glm::transform(r, inv);
}

static void orbit_camera_pan(glm::vec3 vec) {
  float d = orbit_camera.distance;
  glm::vec3 scratch;
  scratch[0] = -d * vec[0];
  scratch[1] =  d * vec[1];
  scratch[2] =  d * vec[2];
  // TODO: Busted
  // orbit_camera.center = orbit_camera.center + glm::vec3_transform_quat(scratch, orbit_camera.rotation);
}
*/

static void orbit_camera_zoom(const float amount) {
  orbit_camera.distance += amount;
}

static glm::mat4 fromRotationTranslation(glm::quat q, glm::vec3 v) {

  // Quaternion math
  float x = q[0], y = q[1], z = q[2], w = q[3],
    x2 = x + x,
    y2 = y + y,
    z2 = z + z,

    xx = x * x2,
    xy = x * y2,
    xz = x * z2,
    yy = y * y2,
    yz = y * z2,
    zz = z * z2,
    wx = w * x2,
    wy = w * y2,
    wz = w * z2;
  glm::mat4 out;

  return out;
/*
  out[0] = 1 - (yy + zz);
  out[1] = xy + wz;
  out[2] = xz - wy;
  out[3] = 0;
  out[4] = xy - wz;
  out[5] = 1 - (xx + zz);
  out[6] = yz + wx;
  out[7] = 0;
  out[8] = xz + wy;
  out[9] = yz - wx;
  out[10] = 1 - (xx + yy);
  out[11] = 0;
  out[12] = v[0];
  out[13] = v[1];
  out[14] = v[2];
  out[15] = 1;
*/
  return out;

}

static glm::mat4 orbit_camera_view() {
  glm::vec3 s = glm::vec3(0.0, 0.0, -orbit_camera.distance);
  glm::quat q = glm::conjugate(orbit_camera.rotation);

  glm::mat4 rot = glm::mat4_cast(q);
  glm::mat4 trans = glm::translate(glm::mat4(1.0f), s);
  glm::mat4 view = trans * rot;
  return view;
}

#endif
