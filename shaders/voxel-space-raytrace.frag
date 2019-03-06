#version 450 core
#extension GL_NV_gpu_shader5: require
#extension GL_ARB_bindless_texture : require

#include "ray-aabb.glsl"
#include "dda-cursor.glsl"
#include "palette.glsl"
#include "cosine-direction.glsl"

in vec3 in_ray_dir;
layout(location = 0) out vec4 outColor;
layout (std430, binding=1) buffer volumeSlab {
  uint8_t volume[];
};
// main binds
uniform float debug;
uniform int showHeat;
uniform float maxDistance;
uniform vec3 eye;
uniform vec3 dims;
uniform uint time;

#define ITERATIONS 900

bool voxel_get(vec3 pos, out uint8_t palette_idx) {
  if (any(lessThan(pos, ivec3(0))) || any(greaterThanEqual(pos, ivec3(dims)))) {
    return false;
  }

  uint idx = uint(
    pos.x +
    pos.y * dims.x +
    pos.z * dims.x * dims.y
  );

  palette_idx = volume[idx];

  return palette_idx > uint8_t(0);
}

void bounce(in out DDACursor cursor, vec3 normal) {
  vec3 new_ray_dir = normalize( (reflect(cursor.rayDir, normal)));
  vec3 hd = dims * 0.5;
  // cursor = dda_cursor_create(
  //   cursor.mapPos - hd + normal,// + new_ray_dir * 0.00001,
  //   vec3(0.0),
  //   hd,
  //   new_ray_dir
  // );
  new_ray_dir += cosine_direction(
    cosine_hash(dot(new_ray_dir, normal)),// * float(time) / 1000.0),
    normal
  ) * 0.07;

  new_ray_dir = normalize(new_ray_dir);

  cursor = dda_cursor_create(
    (cursor.mapPos - hd) + normal + new_ray_dir * .01,
    vec3(0.0),
    hd,
    new_ray_dir
  );
}


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

    for (int i=0; i<ITERATIONS; i++) {
      if (voxel_get(cursor.mapPos, palette_idx)) {
        vec3 c = palette_color(palette_idx);
        //outColor = vec4(cursor.mask, 1.0);

        // vec4 o = vec4(1.0);
        //
        // if (cursor.mask.x > 0.0) {
        //   o *= 0.75;
        // }
        //
        // if (cursor.mask.y > 0.0) {
        //   o *= 0.5;
        // }
        //
        // if (cursor.mask.z > 0.0) {
        //   o *= 0.2;
        // }

        if (bounced == 0.0) {

          if (cursor.mapPos.y > 9) {
            outColor = vec4(c, 1.0);
            break;
          }
        } else {
          //outColor = vec4(cursor.mask, 1.0);//vec4(0.1, 0.1, 0.00, 1.0);// * 0.005;
          outColor += vec4(c, 1.0) * 0.2;//vec4(0.1, 0.1, 0.00, 1.0);// * 0.005;
          break;
        }
        //if (cursor.mapPos.y < 20) {
        bounced++;
        bounce(cursor, cursor.mask);
        //outColor = vec4(bounce(cursor, cursor.mask), 1.0);
        //}
        agg += cursor.mask;
        if (bounced > 2.0) {
          break;
        }
      }

      dda_cursor_step(cursor, found_normal);
    }
  }
}
