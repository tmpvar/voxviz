#version 450 core
#extension GL_NV_gpu_shader5: require
#extension GL_ARB_bindless_texture : require

#include "ray-aabb.glsl"
#include "dda-cursor.glsl"
#include "palette.glsl"
#include "cosine-direction.glsl"

in vec3 in_ray_dir;
layout(location = 0) out vec4 outColor;

// main binds
uniform float debug;
uniform int showHeat;
uniform float maxDistance;
uniform vec3 eye;
uniform vec3 dims;
uniform uint time;

uniform uvec3 lightPos;
uniform vec3 lightColor;

#define ITERATIONS 768

#include "voxel-space-mips.glsl"

void bounce(in DDACursor cursor, vec3 normal) {
  vec3 cd = cosine_direction(
    cosine_hash(
      dot(cursor.mapPos, cursor.rayPos) *
      float(time) / 1000.0
    ),
    normal
  );

  vec3 new_ray_dir = normalize( (reflect(cursor.rayDir, normal)));
  vec3 hd = dims * 0.5;
  cursor = dda_cursor_create(
    cursor.mapPos - hd + normal,// + new_ray_dir * 0.00001,
    vec3(0.0),
    hd,
    new_ray_dir
  );
  // new_ray_dir += cosine_direction(
  //   cosine_hash(dot(new_ray_dir, normal)),// * float(time) / 1000.0),
  //   new_ray_dir
  // ) * 0.05;

  // new_ray_dir = normalize(new_ray_dir);
  //
  // cursor = dda_cursor_create(
  //   (cursor.mapPos - hd) + sign(new_ray_dir) * normalize(normal) + new_ray_dir,
  //   vec3(0.0),
  //   hd,
  //   new_ray_dir
  // );
}

float trace_light(in DDACursor cursor) {
  vec3 normal = vec3(cursor.mask);
  float div = 2.0;
  vec3 mapPos = cursor.mapPos / div;
  vec3 hd = (dims * 0.5) / div;

  vec3 cd = cosine_direction(
    cosine_hash(
      dot(cursor.mapPos, cursor.rayPos) *
      float(time) / 1000.0
    ),
    normal
  );

  vec3 new_ray_dir = cd * vec3(
    length(cd),
    (55 + sin(float(time)/100.00) * 50) / div,
    0.0
  ) - mapPos / div;
  new_ray_dir = normalize(new_ray_dir);

  DDACursor c = dda_cursor_create(
    (mapPos - hd + sign(new_ray_dir) * normal + new_ray_dir),
    vec3(0.0),
    hd,
    new_ray_dir
  );

  uint8_t noop;
  vec3 found_normal;
  for (int i=0; i<128; i++) {
    if (voxel_mip_get(c.mapPos, 1, noop)) {
      return -1.0;
    }
    dda_cursor_step(c, found_normal);
  }
  return 1.0;
}

float trace_sky(in DDACursor cursor) {
  vec3 normal = vec3(cursor.mask);
  float div = 2.0;
  vec3 mapPos = cursor.mapPos / div;
  vec3 hd = (dims * 0.5) / div;

  vec3 cd = cosine_direction(
    cosine_hash(
      dot(cursor.mapPos, cursor.rayPos) *
      float(time) / 1000.0
    ),
    normal
  );

  vec3 new_ray_dir = cd * vec3(
    0.0,
    1.0,
    0.0
  );
  new_ray_dir = normalize(new_ray_dir);

  DDACursor c = dda_cursor_create(
    (mapPos - hd + sign(new_ray_dir) * normal + new_ray_dir * 2) + cd,
    vec3(0.0),
    hd,
    new_ray_dir
  );

  uint8_t noop;
  vec3 found_normal;
  for (int i=0; i<20; i++) {
    if (voxel_mip_get(c.mapPos, 1, noop)) {
      return -1.0;
    }
    dda_cursor_step(c, found_normal);
  }
  return 1.0;
}


vec3 trace_reflection(in DDACursor cursor) {
  vec3 hd = (dims * 0.5) / 2;
  vec3 normal = normalize(vec3(cursor.mask));

  vec3 mapPos = cursor.mapPos / 2.0;
  vec3 cd = cosine_direction(
    cosine_hash(
      dot(mapPos, cursor.rayPos) *
      float(time) / 100.0
    ),
    normal
  ) * 0.3;

  vec3 new_ray_dir = reflect(normalize(cursor.rayDir) + cd, normal);
  new_ray_dir = normalize(new_ray_dir);

  DDACursor c = dda_cursor_create(
    //(mapPos - hd + sign(new_ray_dir) * normal + new_ray_dir * 2)
    (mapPos - hd) + sign(new_ray_dir) * normal + new_ray_dir,// + cd,
    vec3(0.0),
    hd,
    new_ray_dir
  );

  uint8_t noop;
  vec3 found_normal;
  for (int i=0; i<64; i++) {
    if (voxel_mip_get(c.mapPos, 1, noop)) {
      return c.mapPos * 2;
    }
    dda_cursor_step(c, found_normal);
  }
  return vec3(-1.0);
}

// float sum(vec3 v) { return dot(v, vec3(1.0)); }
// float vertex_ao(vec2 side, float corner) {
// 	//if (side.x == 1.0 && side.y == 1.0) return 1.0;
// 	return (side.x + side.y + max(corner, side.x * side.y)) / 3.0;
// }
//
// vec4 voxel_ao(vec3 pos, vec3 d1, vec3 d2) {
//   uint8_t noop;
// 	vec4 side = vec4(
//     float(voxel_get(pos + d1, noop)),
//     float(voxel_get(pos + d2, noop)),
//     float(voxel_get(pos - d1, noop)),
//     float(voxel_get(pos - d2, noop))
//   );
// 	vec4 corner = vec4(
//     float(voxel_get(pos + d1 + d2, noop)),
//     float(voxel_get(pos - d1 + d2, noop)),
//     float(voxel_get(pos - d1 - d2, noop)),
//     float(voxel_get(pos + d1 - d2, noop))
//   );
// 	vec4 ao;
// 	ao.x = vertex_ao(side.xy, corner.x);
// 	ao.y = vertex_ao(side.yz, corner.y);
// 	ao.z = vertex_ao(side.zw, corner.z);
// 	ao.w = vertex_ao(side.wx, corner.w);
// 	return 1.0 - ao;
// }

void main() {
  float found_distance;
  vec3 found_normal;
  vec3 hd = dims * 0.5;
  bool hit = ailaWaldHitAABox(
    vec3(0.0),
    hd,
    eye,
    in_ray_dir,
    1.0 / in_ray_dir,
    found_distance,
    found_normal
  );

  outColor = vec4(0.2);

  if (hit) {
    // outColor = vec4(0.0, 0.0, 0.2, 1.0);
    vec3 pos = eye + in_ray_dir * found_distance;
    if (all(greaterThanEqual(eye, -hd)) && all(lessThanEqual(eye, hd))) {
      pos = eye;
    }

    //vec3 pos = (eye + in_ray_dir * found_distance) * dims;
    DDACursor cursor = dda_cursor_create(
      pos - in_ray_dir * 0.0001,
      vec3(0.0),
      hd,
      in_ray_dir
    );
    float bounced = 0.0;
    vec3 agg;
    uint8_t palette_idx;

    bool lighting = false;
    vec3 c = vec3(0.2);
    float intensity = 0.0;//0.67;
    vec3 reflectColor = vec3(0.0);
    for (int i=0; i<ITERATIONS; i++) {
      if (voxel_mip_get(cursor.mapPos, 0, palette_idx)) {
        // intensity += trace_light(cursor) * 0.55;
        //intensity += trace_sky(cursor) * 0.75;
        if (cursor.mapPos.y < 10) {
          vec3 rpos = trace_reflection(cursor);
          if (rpos.x >= 0.0 && voxel_mip_get(rpos, 0, palette_idx)) {
            reflectColor = palette_color(palette_idx);// / 2.0;
          }
        } else {
            c = palette_color(palette_idx);
        }
        break;
      }

      dda_cursor_step(cursor, found_normal);
    }

    outColor = vec4(c + reflectColor * 0.2 + intensity * 0.2 * lightColor, 1.0);
  }
}
