#version 330 core

out vec4 outColor;
in vec2 uv;

uniform float maxDistance;

uniform sampler2D iColor;
uniform sampler2D iPosition;

// TODO: make this an array for multiple lights
uniform vec3 light;
uniform sampler2DShadow iShadowMap;
//uniform sampler2D iShadowMap;
uniform sampler2D iShadowColor;
uniform sampler2D iShadowPosition;

uniform mat4 depthBiasMVP;
uniform vec2 shadowmapResolution;
uniform vec3 eye;


float shadow(vec2 uv, float d) {
  return texture(iShadowMap, vec3(uv, d));
}

void main() {
  vec3 pos = texture(iPosition, uv).xyz * maxDistance;
  vec4 s = depthBiasMVP * vec4(pos, 1.0);
  vec3 color = texture(iColor, uv).xyz;
  outColor = texture(iColor, uv);
  return;
  float dd = dot(pos - light, light - color);

  // float bias = 0.001;//clamp(0.0001 + dd, 0.0001, 0.0016);

  float bias = 0.00012;// * tan(acos(-dot(color, normalize(pos - light))));
  bias = clamp(bias, 0, 0.01);

  vec2 shadowUV =  s.xy / s.w;

  float distanceToLight = distance(pos, light) / maxDistance;
  float intensity = 1.0 - (distanceToLight / 10000000.0);
  float visibility = shadow(shadowUV, distanceToLight - bias) * intensity;

  if (any(lessThan(shadowUV, vec2(0.0))) || any(greaterThanEqual(shadowUV, vec2(1.0)))) {
    outColor = 0.1 * vec4(color, 1.0);
  } else {
    outColor = vec4(color.rgb * max(0.1, visibility), 1.0);

    //outColor = vec4(dd);
  }

  vec2 depthBox = vec2(1.0 - 0.125);
  if (all(greaterThanEqual(uv, depthBox))) {
    // outColor = vec4(texture(iShadowMap, (uv - colorBox)  / 0.125).rgb, 1.0);
    float ld = texture(
      iShadowMap, vec3((uv - depthBox)  / 0.125,
      distanceToLight)
    );
    outColor = vec4(1.0 - ld);
    return;
  }
}
