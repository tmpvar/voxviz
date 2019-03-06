#version 430 core

in vec3 position;
uniform mat4 VP;
out vec3 in_ray_dir;

void main() {
  mat4 inv = inverse(VP);

  vec4 far = inv * vec4(position.x, position.y, 1.0, 1.0);
  far /= far.w;
  vec4 near = inv * vec4(position.x, position.y, 0.1, 1.0);
  near /= near.w;
  in_ray_dir = far.xyz - near.xyz;

  gl_Position = vec4(position, 1.0);
}
