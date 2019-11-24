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


void stupid_mesher(const u8 *volume, const ivec3 &dims, Mesh *mesh) {
  u32 voxel_idx = 0;

  for (uint x = 0; x < dims.x; x++) {
    for (uint y = 0; y < dims.y; y++) {
      for (uint z = 0; z < dims.z; z++) {

        // the current voxel is set
        if (volume[x + y * dims.x + z * dims.x*dims.y]) {
          uint c = static_cast<GLuint>(mesh->verts.size() / 3);
          mesh
            ->face(c + 0, c + 1, c + 2)
            ->face(c + 0, c + 2, c + 3)

            ->face(c + 4, c + 6, c + 5)
            ->face(c + 4, c + 7, c + 6)

            ->face(c + 8, c + 10, c + 9)
            ->face(c + 8, c + 11, c + 10)

            ->face(c + 12, c + 13, c + 14)
            ->face(c + 12, c + 14, c + 15)

            ->face(c + 16, c + 18, c + 17)
            ->face(c + 16, c + 18, c + 19)

            ->face(c + 20, c + 21, c + 22)
            ->face(c + 20, c + 22, c + 23);

          mesh
            ->vert(x + 0, y + 0, z + 1)
            ->vert(x + 1, y + 0, z + 1)
            ->vert(x + 1, y + 1, z + 1)
            ->vert(x + 0, y + 1, z + 1)
            ->vert(x + 1, y + 1, z + 1)
            ->vert(x + 1, y + 1, z + 0)
            ->vert(x + 1, y + 0, z + 0)
            ->vert(x + 1, y + 0, z + 1)
            ->vert(x + 0, y + 0, z + 0)
            ->vert(x + 1, y + 0, z + 0)
            ->vert(x + 1, y + 1, z + 0)
            ->vert(x + 0, y + 1, z + 0)
            ->vert(x + 0, y + 0, z + 0)
            ->vert(x + 0, y + 0, z + 1)
            ->vert(x + 0, y + 1, z + 1)
            ->vert(x + 0, y + 1, z + 0)
            ->vert(x + 1, y + 1, z + 1)
            ->vert(x + 0, y + 1, z + 1)
            ->vert(x + 0, y + 1, z + 0)
            ->vert(x + 1, y + 1, z + 0)
            ->vert(x + 0, y + 0, z + 0)
            ->vert(x + 1, y + 0, z + 0)
            ->vert(x + 1, y + 0, z + 1)
            ->vert(x + 0, y + 0, z + 1);
        }
      }
    }
  }
}


static bool isset(ivec3 p, const u8 *volume, const ivec3 &dims) {
  if (glm::any(glm::lessThan(p, ivec3(0))) || glm::any(glm::greaterThanEqual(p, dims))) {
    return false;
  }
  return volume[p.x + p.y * dims.x + p.z * dims.x * dims.y] > 0;
}

void culled_mesher(const u8 *volume, const ivec3 &dims, Mesh *mesh) {

  u32 voxel_idx = 0;
  uvec3 pos;
  vec3 hd = vec3(dims) * vec3(0.5);
  for (pos.z = 0; pos.z < dims.z; pos.z++) {
    for (pos.y = 0; pos.y < dims.y; pos.y++) {
      for (pos.x = 0; pos.x < dims.x; pos.x++) {
        // the current voxel is set
        if (isset(pos, volume, dims)) {
          // loop over the cardinal directions
          // with the intent of generating a quad for every face
          // X (0), Y (1), Z (2)
          for (int d = 0; d < 3; d++) {
            uvec3 corner = pos;

            uvec3 u(0, 0, 0);
            uvec3 v(0, 0, 0);

            u[(d + 1) % 3] = 1;
            v[(d + 2) % 3] = 1;

            for (int side = 0; side < 2; side++) {
              if (side == 0) {
                ivec3 o = pos;
                o[d]-=1;
                if (isset(o, volume, dims)) {
                  continue;
                }
              } else {
                uvec3 o = pos;
                o[d] += 1;
                if (isset(o, volume, dims)) {
                  continue;
                }
              }

              corner[d] = pos[d] + side;

              u32 start = static_cast<GLuint>(mesh->verts.size() / 3);
              mesh->vert(
                vec3(corner) - hd
              );

              mesh->vert(
                vec3(corner + u) - hd
              );

              mesh->vert(
                vec3(corner + u + v) - hd
              );

              mesh->vert(
                vec3(corner + v) - hd
              );

              // compute side normal
              {
                vec3 normal(0.0);
                normal[d] = (side == 0) ? 1.0 : -1.0;
                mesh->normal(normal.x, normal.y, normal.z);
                // TODO: ummm.. there's some weirdness that I don't have the
                // brain power to solve right now with glVertexAttribDivisor
                // in the model render method that needs addressing to avoid
                // duplicating this data.
                mesh->normal(normal.x, normal.y, normal.z);
                mesh->normal(normal.x, normal.y, normal.z);
                mesh->normal(normal.x, normal.y, normal.z);
              }

              if (side == 1) {
                mesh->face(start, start + 1, start + 2)
                    ->face(start, start + 2, start + 3);
              }
              else {
                mesh->face(start + 2, start + 1, start)
                    ->face(start + 3, start + 2, start);
              }
            }
          }
        }

        voxel_idx++;
      }
    }
  }

}

// taken from https://github.com/mikolalysenko/mikolalysenko.github.com/blob/master/MinecraftMeshes/js/greedy.js
void greedy_mesher(const u8 *volume, const ivec3 &dims, Mesh *mesh) {
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
            //mesh->face(vertex_count + 2, vertex_count + 1, vertex_count)
            //  ->face(vertex_count+3, vertex_count + 2, vertex_count);

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