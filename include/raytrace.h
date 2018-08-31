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
  #include "core.h"

  using namespace std;

  class Raytracer {
    public:
    Mesh *mesh;
    Program *program;
    int *dims;
    GLuint volumeTexture;
    int showHeat;
    vector<Volume *> volumes;
    

    Raytracer(int *dimensions) {
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

      //glDeleteBuffers(1, &instanceVBO); gl_error();
      //glDeleteBuffers(1, &pointerVBO); gl_error();
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

      volume->upload();

      this->volumes.push_back(volume);
      // TODO: add this volume to a spacial hash

      return volume;
    }

    void upload() {
      size_t total_bricks = this->volumes.size();
      if (total_bricks == 0) {
        return;
      }

      size_t total_position_mem = total_bricks * 3 * sizeof(float);
      size_t total_volume_pointer_mem = total_bricks * sizeof(GLuint64);
      float *positions = (float *)malloc(total_position_mem);
      GLuint64 *pointers = (GLuint64 *)malloc(total_volume_pointer_mem);
      size_t loc = 0;
      for (auto& volume : this->volumes) {
        positions[loc * 3 + 0] = volume->center.x;
        positions[loc * 3 + 1] = volume->center.y;
        positions[loc * 3 + 2] = volume->center.z;

        pointers[loc] = volume->bufferAddress;
        loc++;
      }
      

      // Mesh data
      glBindVertexArray(this->mesh->vao);
      glEnableVertexAttribArray(0);
      glBindBuffer(GL_ARRAY_BUFFER, this->mesh->vbo);

      // Instance data
      unsigned int instanceVBO;
      glGenBuffers(1, &instanceVBO); gl_error();
      glEnableVertexAttribArray(1); gl_error();
      glBindBuffer(GL_ARRAY_BUFFER, instanceVBO); gl_error();
      glBufferData(GL_ARRAY_BUFFER, total_position_mem, &positions[0], GL_STATIC_DRAW); gl_error();
      glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0); gl_error();
      glVertexAttribDivisor(1, 1); gl_error();
      
      
      // Buffer pointer data
      unsigned int pointerVBO;
      glGenBuffers(1, &pointerVBO); gl_error();
      glEnableVertexAttribArray(2); gl_error();
      glBindBuffer(GL_ARRAY_BUFFER, pointerVBO); gl_error();
      glBufferData(GL_ARRAY_BUFFER, total_volume_pointer_mem, &pointers[0], GL_STATIC_DRAW); gl_error();
      glVertexAttribLPointer(2, 1, GL_UNSIGNED_INT64_ARB, 0, 0); gl_error();
      glVertexAttribDivisor(2, 1); gl_error();

      free(positions);
      free(pointers);
    }

    void render (Program *p) {
      p->uniformVec3ui("dims", glm::uvec3(VOLUME_DIMS, VOLUME_DIMS, VOLUME_DIMS));
      
      glBindVertexArray(this->mesh->vao);

      glDrawElementsInstanced(
        GL_TRIANGLES,
        this->mesh->faces.size(),
        GL_UNSIGNED_INT,
        0,
        this->volumes.size()
      );
       gl_error();
    }
  };

#endif
