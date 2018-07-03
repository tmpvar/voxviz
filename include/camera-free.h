#ifndef __CAMERA_FREE_H
#define __CAMERA_FREE_H

 #include <glm/glm.hpp>

 class FreeCamera {
   glm::vec3 m_pos;
   glm::quat m_orient;
 public:
  FreeCamera(void) = default;

  const glm::vec3 &position(void) const { return m_pos; }
  const glm::quat &orientation(void) const { return m_orient; }

  glm::mat4 view(void) const { return glm::translate(glm::mat4_cast(m_orient), m_pos); }

  void translate(const glm::vec3 &v) {
    m_pos += v * m_orient;
  }

  void translate(float x, float y, float z) {
    m_pos += glm::vec3(x, y, z) * m_orient;
  }

  void rotate(float angle, float x, float y, float z) {
    m_orient *= glm::angleAxis(angle, glm::vec3(x, y, z) * m_orient);
  }

  void yaw(float angle) { rotate(angle, 0.0f, 1.0f, 0.0f); }
  void pitch(float angle) { rotate(angle, 1.0f, 0.0f, 0.0f); }
  void roll(float angle) { rotate(angle, 0.0f, 0.0f, 1.0f); }
};

#endif
