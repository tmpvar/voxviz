#version 150 core

in vec3 position;
uniform mat4 MVP;
uniform ivec3 dims;
uniform vec3 center;

out vec3 rayOrigin;

out vec3 aabb_lb;
out vec3 aabb_ub;

void main() {
  vec3 pos = position * vec3(dims/2.0);

  rayOrigin = pos + center;

  aabb_lb = -dims/2.0;
  aabb_ub =  dims/2.0;

  gl_Position = MVP * vec4(rayOrigin, 1.0);
}
