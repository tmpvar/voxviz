#version 330 core

out vec4 outColor;
in vec2 uv;

uniform float maxDistance;

uniform sampler2D iColor;
uniform sampler2D iPosition;

// TODO: make this an array for multiple lights
uniform vec3 light;
uniform sampler2DShadow iShadowMap;
uniform mat4 depthBiasMVP;
uniform vec2 shadowmapResolution;
uniform vec3 eye;

float shadow(vec2 uv, float d) {
  return texture(iShadowMap, vec3(uv, d));
}

void main() {
  vec3 pos = texture(iPosition, uv).xyz * maxDistance;
  vec3 normal = texture(iColor, uv).xyz;
  vec4 s = depthBiasMVP * vec4(pos, 1.0);

  float bias = 0.005;
  vec2 shadowUV = s.xy / s.w;

  float distanceToLight = distance(pos, light) / maxDistance;
  float intensity = clamp(0.5 - distance(shadowUV, vec2(0.5)), 0.0, 0.5);

  float visibility = shadow(shadowUV, distanceToLight - bias) * intensity;

  outColor = vec4(normal * (0.1 + visibility), 1.0);
}