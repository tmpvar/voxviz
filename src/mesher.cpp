#include "mesher.h"

#include <string.h>
#include <stdio.h>

#define f(i, j, k) volume[i + dims[0] * (j + dims[1] * k)]

#define add_quad(v00, v01, v02, v10, v11, v12, v20, v21, v22, v30, v31, v32) \
  const std::size_t vertex_count = vertices.size()/3; \
  vertices.push_back(hdim[0] + v00); vertices.push_back(hdim[1] + v01); vertices.push_back(hdim[2] + v02); \
  vertices.push_back(hdim[0] + v10); vertices.push_back(hdim[1] + v11); vertices.push_back(hdim[2] + v12); \
  vertices.push_back(hdim[0] + v20); vertices.push_back(hdim[1] + v21); vertices.push_back(hdim[2] + v22); \
  vertices.push_back(hdim[0] + v30); vertices.push_back(hdim[1] + v31); vertices.push_back(hdim[2] + v32); \
  faces.push_back(vertex_count + 0); \
  faces.push_back(vertex_count + 1); \
  faces.push_back(vertex_count + 2); \
  faces.push_back(vertex_count + 2); \
  faces.push_back(vertex_count + 3); \
  faces.push_back(vertex_count + 0);

// using the stupid mesher from http://mikolalysenko.github.io/MinecraftMeshes2/js/stupid.js
void vx_stupid_mesher(
  const int *volume,
  const int *dims,
  std::vector<float> &vertices,
  std::vector<unsigned int> &faces
) {
  int hdim[3] = {-dims[0]/2,-dims[1]/2,-dims[2]/2};
  int x[3] = {0, 0, 0}, n = 0;
  for(x[2]=0; x[2]<dims[2]; ++x[2]) {
    for(x[1]=0; x[1]<dims[1]; ++x[1]) {
      for(x[0]=0; x[0]<dims[0]; ++x[0], ++n) {
        if(volume[n]) {
          for(int d=0; d<3; ++d) {
            int t[3] = {x[0], x[1], x[2]}
              , u[3] = {0,0,0}
              , v[3] = {0,0,0};
            u[(d+1)%3] = 1;
            v[(d+2)%3] = 1;
            for(int s=0; s<2; ++s) {
              t[d] = x[d] + s;
              int tmp[3];
              memcpy(tmp, u, sizeof(u));
              memcpy(u, v, sizeof(u));
              memcpy(v, tmp, sizeof(u));

              add_quad(
                t[0], t[1], t[2],
                t[0] + u[0], t[1] + u[1], t[2] + u[2],
                t[0] + u[0] + v[0], t[1] + u[1] + v[1], t[2] + u[2] + v[2],
                t[0] + v[0], t[1] + v[1], t[2] + v[2]
              )
            }
          }
        }
      }
    }
  }
}

void vx_greedy_mesher(
  const int *volume,
  const int *dims,
  std::vector<float> &vertices,
  std::vector<unsigned int> &faces
) {

  for(int d=0; d<3; ++d) {
    int i, j, k, l, w, h
      , u = (d+1)%3
      , v = (d+2)%3
      , x[3] = {0,0,0}
      , q[3] = {0,0,0};

    int *mask = (int *)malloc(sizeof(int) * dims[u] * dims[v]);

    q[d] = 1;
    for(x[d]=-1; x[d]<dims[d]; ) {
      //Compute mask
      int n = 0;
      for(x[v]=0; x[v]<dims[v]; ++x[v]) {
        for(x[u]=0; x[u]<dims[u]; ++x[u], ++n) {
          int a = (0    <= x[d]      ? f(x[0],      x[1],      x[2])      : 0);
          int b = (x[d] <  dims[d]-1 ? f(x[0]+q[0], x[1]+q[1], x[2]+q[2]) : 0);
          if((!!a) == (!!b)) {
            mask[n] = 0;
          } else if(!!a) {
            mask[n] = a;
          } else {
            mask[n] = -b;
          }
        }
      }

      //Increment x[d]
      ++x[d];
      //Generate mesh for mask using lexicographic ordering
      n = 0;
      for(j=0; j<dims[v]; ++j) {
        for(i=0; i<dims[u]; ) {
          int c = mask[n];
          if(!!c) {
            //Compute width
            for(w=1; c == mask[n+w] && i+w<dims[u]; ++w) {
            }
            //Compute height (this is slightly awkward
            int done = false;
            for(h=1; j+h<dims[v]; ++h) {
              for(k=0; k<w; ++k) {
                if(c != mask[n+k+h*dims[u]]) {
                  done = true;
                  break;
                }
              }
              if(done) {
                break;
              }
            }
            //Add quad
            x[u] = i;
            x[v] = j;
            int du[3] = {0,0,0};
            int dv[3] = {0,0,0};
            if(c > 0) {
              dv[v] = h;
              du[u] = w;
            } else {
              c = -c;
              du[v] = h;
              dv[u] = w;
            }

            const std::size_t vertex_count = vertices.size() / 3;

            vertices.push_back(x[0]);
            vertices.push_back(x[1]);
            vertices.push_back(x[2]);

            vertices.push_back(x[0] + du[0]);
            vertices.push_back(x[1] + du[1]);
            vertices.push_back(x[2] + du[2]);

            vertices.push_back(x[0] + du[0] + dv[0]);
            vertices.push_back(x[1] + du[1] + dv[1]);
            vertices.push_back(x[2] + du[2] + dv[2]);

            vertices.push_back(x[0] + dv[0]);
            vertices.push_back(x[1] + dv[1]);
            vertices.push_back(x[2] + dv[2]);

            faces.push_back(vertex_count + 0);
            faces.push_back(vertex_count + 1);
            faces.push_back(vertex_count + 2);

            faces.push_back(vertex_count + 2);
            faces.push_back(vertex_count + 3);
            faces.push_back(vertex_count + 0);
            // faces.push_back(vector3(vertex_count, vertex_count + 1, vertex_count + 2));
            // faces.push_back(vector3(vertex_count + 2, vertex_count + 3, vertex_count));

            //Zero-out mask
            for(l=0; l<h; ++l) {
              for(k=0; k<w; ++k) {
                mask[n+k+l*dims[u]] = 0;
              }
            }

            //Increment counters and continue
            i += w;
            n += w;
          } else {
            ++i;
            ++n;
          }
        }
      }
    }
    free(mask);
  }
}

void vx2_mesher(
  const int *volume,
  const int *dims,
  std::vector<float> &vertices,
  std::vector<unsigned int> &faces
) {
  for (std::size_t axis = 0; axis < 3; ++axis)
  {
    // printf("axis: %ld\n", axis);

    const std::size_t u = (axis + 1) % 3;
    const std::size_t v = (axis + 2) % 3;

    // printf("u: %ld, v: %ld\n", u, v);

    int x[3] = { 0 }, q[3] = { 0 };
    int *mask = (int *)malloc(sizeof(int) * dims[u] * dims[v]);
    // printf("dims[u]: %d, dims[v]: %d\n", dims[u], dims[v]);

    // printf("x: %d, %d, %d\n", x[0], x[1], x[2]);
    // printf("q: %d, %d, %d\n", q[0], q[1], q[2]);

    // Compute mask
    q[axis] = 1;
    for (x[axis] = -1; x[axis] < dims[axis];)
    {
      // printf("x: %d, %d, %d\n", x[0], x[1], x[2]);
      // printf("q: %d, %d, %d\n", q[0], q[1], q[2]);

      std::size_t counter = 0;
      for (x[v] = 0; x[v] < dims[v]; ++x[v])
        for (x[u] = 0; x[u] < dims[u]; ++x[u], ++counter)
        {
          const int a = 0 <= x[axis] ? f(x[0], x[1], x[2]) : 0;
          const int b = x[axis] < dims[axis] - 1 ? f(x[0] + q[0],
                                                     x[1] + q[1],
                                                     x[2] + q[2]) : 0;
          const bool ba = static_cast<bool>(a);
          if (ba == static_cast<bool>(b))
            mask[counter] = 0;
          else if (ba)
            mask[counter] = a;
          else
            mask[counter] = -b;
        }

      ++x[axis];

      // for (size_t a = 0; a < dims[u] * dims[v]; ++a)
      //   printf("%d, ", mask[a]);
      // printf("\n");

      // Generate mesh for mask using lexicographic ordering
      std::size_t width = 0, height = 0;

      counter = 0;
      for (std::size_t j = 0; j < dims[v]; ++j)
        for (std::size_t i = 0; i < dims[u];)
        {
          int c = mask[counter];
          if (c)
          {
            // Compute width
            for (width = 1; c == mask[counter + width] &&
                   i + width < dims[u]; ++width)
            {}

            // Compute height
            bool done = false;
            for (height = 1; j + height < dims[v]; ++height)
            {
              for (std::size_t k = 0; k < width; ++k)
                if (c != mask[counter + k + height * dims[u]])
                {
                  done = true;
                  break;
                }

              if (done)
                break;
            }

            // Add quad
            x[u] = i;
            x[v] = j;

            int du[3] = {0}, dv[3] = {0};

            if (c > 0)
            {
              dv[v] = height;
              du[u] = width;
            }
            else
            {
              c = -c;
              du[v] = height;
              dv[u] = width;
            }

            // const std::size_t vertex_count = vertices.size();
            // vertices.push_back(vector3(x[0], x[1], x[2]));
            // vertices.push_back(vector3(x[0] + du[0], x[1] + du[1], x[2] + du[2]));
            // vertices.push_back(vector3(x[0] + du[0] + dv[0], x[1] + du[1] + dv[1], x[2] + du[2] + dv[2]));
            // vertices.push_back(vector3(x[0] + dv[0], x[1] + dv[1], x[2] + dv[2]));

            // faces.push_back(vector3(vertex_count, vertex_count + 1, vertex_count + 2));
            // faces.push_back(vector3(vertex_count, vertex_count + 2, vertex_count + 3));

            const std::size_t vertex_count = vertices.size() / 3;

            vertices.push_back(x[0]);
            vertices.push_back(x[1]);
            vertices.push_back(x[2]);

            vertices.push_back(x[0] + du[0]);
            vertices.push_back(x[1] + du[1]);
            vertices.push_back(x[2] + du[2]);

            vertices.push_back(x[0] + du[0] + dv[0]);
            vertices.push_back(x[1] + du[1] + dv[1]);
            vertices.push_back(x[2] + du[2] + dv[2]);

            vertices.push_back(x[0] + dv[0]);
            vertices.push_back(x[1] + dv[1]);
            vertices.push_back(x[2] + dv[2]);

            faces.push_back(vertex_count + 0);
            faces.push_back(vertex_count + 1);
            faces.push_back(vertex_count + 2);

            faces.push_back(vertex_count + 2);
            faces.push_back(vertex_count + 3);
            faces.push_back(vertex_count + 0);


            for (std::size_t b = 0; b < width; ++b) {
              for (std::size_t a = 0; a < height; ++a) {
                mask[counter + b + a * dims[u]] = 0;
              }
            }

            // Increment counters
            i += width; counter += width;
          }
          else
          {
            ++i;
            ++counter;
          }
        }
    }

    // printf("x: %d, %d, %d\n", x[0], x[1], x[2]);
    // printf("q: %d, %d, %d\n", q[0], q[1], q[2]);


    // printf("\n");
    free(mask);
  }
}
