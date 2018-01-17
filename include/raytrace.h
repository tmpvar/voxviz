#ifndef __RAYTRACE_H_
#define __RAYTRACE_H_
  #include "gl-wrap.h"

  #include "shaders/built.h"
  #include <iostream>
  #include <vector>
  #include <algorithm>
  #include <glm/glm.hpp>
  #include <string.h>

  #include "volume.h"
  #include "clu.h"
  #include "core.h"

  using namespace std;

  class Raytracer {
    public:
    Mesh *mesh;
    Program *program;
    int *dims;
    GLbyte *volume;
    GLuint volumeTexture;
    cl_mem volumeMemory[VOLUME_COUNT];
    int showHeat;
    glm::vec3 center;
    vector<Volume *> volumes;

    Raytracer(int *dimensions, clu_job_t job) {
      this->dims = dimensions;
      this->showHeat = 0;
      glm::vec3 hd(dims[0]/2, dims[1]/2, dims[2]/2);

      this->program = new Program();
      this->program
          ->add(Shaders::get("raytrace.vert"))
          ->add(Shaders::get("raytrace.frag"))
          ->output("outColor")
          ->link();

      this->program->use();

      this->mesh = new Mesh();

      this->mesh
        ->face(0, 1, 2)->face(0, 2, 3)
        ->face(4, 6, 5)->face(4, 7, 6)
        ->face(8, 10, 9)->face( 8, 11, 10)
        ->face(12, 13, 14)->face(12, 14, 15)
        ->face(16, 18, 17)->face(19, 18, 16)
        ->face(20, 21, 22)->face(20, 22, 23);

      this->mesh
        ->vert(-1, -1,  1)->vert( 1, -1,  1)->vert( 1,  1,  1)
        ->vert(-1,  1,  1)->vert( 1,  1,  1)->vert( 1,  1, -1)
        ->vert( 1, -1, -1)->vert( 1, -1,  1)->vert(-1, -1, -1)
        ->vert( 1, -1, -1)->vert( 1,  1, -1)->vert(-1,  1, -1)
        ->vert(-1, -1, -1)->vert(-1, -1,  1)->vert(-1,  1,  1)
        ->vert(-1,  1, -1)->vert( 1,  1,  1)->vert(-1,  1,  1)
        ->vert(-1,  1, -1)->vert( 1,  1, -1)->vert(-1, -1, -1)
        ->vert( 1, -1, -1)->vert( 1, -1,  1)->vert(-1, -1,  1)
        ->upload();

      float square = floor(sqrt((float)VOLUME_COUNT));
      float center = (float)square / 2.0f * float(VOLUME_DIMS);
      this->center = glm::vec3(center, 0.0, center);

      int v = 0;
      for (float x = 0; x < VOLUME_SIDE; x++) {
        for (float y = 0; y < VOLUME_SIDE; y++) {
          for (float z = 0; z < VOLUME_SIDE; z++) {
            Volume *volume = new Volume(glm::vec3(
              x*VOLUME_DIMS, y*VOLUME_DIMS, z*VOLUME_DIMS
            ));
            volume->upload(job);

            this->volumeMemory[v] = volume->computeBuffer;
            this->volumes.push_back(volume);
            v++;
          }
        }
      }
    }

    ~Raytracer() {
      delete this->mesh;
      for (int v = 0; v<VOLUME_COUNT; v++) {
        delete this->volumes[v];
      }
    }

    void render (glm::mat4 mvp, glm::vec3 eye) {
      this->program->use();
      this->program
          ->uniformMat4("MVP", mvp)
          ->uniformVec3("eye", eye)
          ->uniformVec3i("dims", this->dims)
          ->uniform1i("showHeat", this->showHeat);
      /*
      sort(
        this->volumes.begin(),
        this->volumes.end(),
        [eye](const Volume *a, const Volume *b) -> bool {
          float ad = glm::distance(eye, a->center);
          float bd = glm::distance(eye, b->center);

          return ad < bd;
        }
      );*/

      for (auto& volume: this->volumes) {
        volume->bind(this->program);
        this->mesh->render(this->program, "position");
      }
    }
  };

#endif
