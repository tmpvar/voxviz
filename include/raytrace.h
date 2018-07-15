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
    GLuint volumeTexture;
    clu_job_t job;
    int showHeat;
    vector<Volume *> volumes;
    

    Raytracer(int *dimensions, clu_job_t job) {
      this->job = job;
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
    }

    ~Raytracer() {
      delete this->mesh;
      this->reset();
    }

	  void reset() {
      size_t total = this->volumes.size();
		  for (size_t v = 0; v<total; v++) {
			  delete this->volumes[v];
		  }
      this->volumes.clear();
	  }

    Volume *addVolumeAtIndex(float x, float y, float z, unsigned int w, unsigned int h, unsigned d) {
      glm::vec3 pos(x*w + w/2.0f, y*h + h / 2.0f, z*d + d / 2.0f);
      glm::uvec3 dims(w, h, d);

      Volume *volume = new Volume(pos, dims);

      volume->upload(this->job);

      this->volumes.push_back(volume);
      // TODO: add this volume to a spacial hash

      return volume;
    }

    void render (glm::mat4 mvp, glm::vec3 eye, float max_distance) {
      this->program->use();
      this->program
        ->uniformMat4("MVP", mvp)
        ->uniformVec3("eye", eye)
        ->uniform1i("showHeat", this->showHeat)
        ->uniformFloat("maxDistance", max_distance);

      sort(
        this->volumes.begin(),
        this->volumes.end(),
        [eye](const Volume *a, const Volume *b) -> bool {
          float ad = glm::distance(eye, a->center);
          float bd = glm::distance(eye, b->center);

          return ad < bd;
        }
      );

      // TODO: batch render
      for (auto& volume: this->volumes) {
        volume->bind(this->program);
        this->mesh->render(this->program, "position");
      }
    }
  };

#endif
