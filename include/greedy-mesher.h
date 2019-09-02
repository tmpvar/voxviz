#pragma once
#include <cstdio>
#include <vector>

#include <glm/glm.hpp>
using namespace glm;

inline i32 f(const u8 *volume, const glm::uvec3 &dims, u32 i, u32 j, u32 k) {
  //return volume[i + j * dims.x + k * dims.x * dims.y];
  return static_cast<i32>(volume[i + dims[0] * (j + dims[1] * k)]);
}

inline void vert(Mesh *mesh, const i32 x, const i32 y, const i32 z) {
  mesh->vert(
    static_cast<float>(x),
    static_cast<float>(y),
    static_cast<float>(z)
  );
}

// taken from https://github.com/mikolalysenko/mikolalysenko.github.com/blob/master/MinecraftMeshes/js/greedy.js
void greedy(const u8 *volume, const ivec3 &dims, Mesh *mesh) {
  for (i32 d = 0; d < 3; ++d) {
    i32 i = 0, j = 0, k = 0, l = 0, w = 0, h = 0
      , u = (d + 1) % 3
      , v = (d + 2) % 3;
    ivec3 x = ivec3(0)
      , q = ivec3(0);

    i32 *mask = new i32[dims[u] * dims[v]];
    q[d] = 1;
    for (x[d] = -1; x[d] < dims[d]; ) {
      //Compute mask
      i32 n = 0;
      for (x[v] = 0; x[v] < dims[v]; ++x[v]) {
        for (x[u] = 0; x[u] < dims[u]; ++x[u]) {
          mask[n++] =
            (0 <= x[d] ? f(volume, dims, x[0], x[1], x[2]) : false) !=
            (x[d] < dims[d] - 1 ? f(volume, dims, x[0] + q[0], x[1] + q[1], x[2] + q[2]) : false);
        }
      }
      //Increment x[d]
      ++x[d];
      //Generate mesh for mask using lexicographic ordering
      n = 0;
      for (j = 0; j < dims[v]; ++j) {
        for (i = 0; i < dims[u]; ) {
          if (mask[n]) {
            //Compute width
            for (w = 1; mask[n + w] && i + w < dims[u]; ++w) {
            }
            //Compute height (this is slightly awkward
            bool done = false;
            for (h = 1; j + h < dims[v]; ++h) {
              for (k = 0; k < w; ++k) {
                if (!mask[n + k + h * dims[u]]) {
                  done = true;
                  break;
                }
              }
              if (done) {
                break;
              }
            }

            //Add quad
            x[u] = i;  x[v] = j;
            ivec3 du = ivec3(0); du[u] = w;
            ivec3 dv = ivec3(0); dv[v] = h;

            GLuint vertex_count = static_cast<GLuint>(mesh->verts.size() / 3);

            vert(mesh,
              x[0],
              x[1],
              x[2]
            );

            vert(mesh,
              x[0] + du[0],
              x[1] + du[1],
              x[2] + du[2]
            );

            vert(mesh,
              x[0] + du[0] + dv[0],
              x[1] + du[1] + dv[1],
              x[2] + du[2] + dv[2]
            );

            vert(mesh,
              x[0] + dv[0],
              x[1] + dv[1],
              x[2] + dv[2]
            );


            mesh->face(vertex_count, vertex_count + 1, vertex_count + 2)
                ->face(vertex_count, vertex_count + 2, vertex_count + 3);
            // TODO: can this work with backface culling??
            mesh->face(vertex_count + 2, vertex_count + 1, vertex_count)
              ->face(vertex_count+3, vertex_count + 2, vertex_count);

            //Zero-out mask
            for (l = 0; l < h; ++l) {
              for (k = 0; k < w; ++k) {
                mask[n + k + l * dims[u]] = false;
              }
            }
            //Increment counters and continue
            i += w; n += w;
          }
          else {
            ++i;    ++n;
          }
        }
      }
    }

    delete mask;
  }
}