#ifndef __RAYTRACE_H_
#define __RAYTRACE_H_
  #include "gl-wrap.h"

  #include "shaders/built.h"
  #include <iostream>
  #include <glm/glm.hpp>
  #include <string.h>

  #include "volume.h"
  #include "cl/clu.h"

  using namespace std;

  #define VOLUME_COUNT 4

  class Raytracer {
    public:
    Mesh *mesh;
    Program *program;
    int *dims;
    GLbyte *volume;
    GLuint volumeTexture;
    cl_mem volumeMemory;
    int showHeat;

    Volume *volumes[VOLUME_COUNT];

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

      for (int v = 0; v<VOLUME_COUNT; v++) {
        this->volumes[v] = new Volume(glm::vec3(0.0, v * float(DIMS), 0.0));
        // // convert the volume into a 3d texture
        // size_t size = dimensions[0] * dimensions[1] * dimensions[2];
        // for (size_t i=0; i<size; i++) {
        //   int val = volume[i];
        //   this->volumes[v]->data[i*3+0] = val;
        //   this->volumes[v]->data[i*3+1] = val;
        //   this->volumes[v]->data[i*3+2] = val;
        // }

        this->volumes[v]->upload(job);
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

      for (int v = 0; v<VOLUME_COUNT; v++) {
        this->volumes[v]->bind(this->program);
        this->mesh->render(this->program, "position");
      }
    }
  };

#endif
