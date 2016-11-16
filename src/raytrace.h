#ifndef __RAYTRACE_H_
#define __RAYTRACE_H_
  #include "gl-wrap.h"

  #include "shaders/built.h"
  #include <iostream>
  #include <glm/glm.hpp>
  #include <string.h>

  using namespace std;


  class Raytracer {
    public:
    Mesh *mesh;
    Program *program;
    int *dims;
    GLbyte *volume;
    GLuint volumeTexture;

    Raytracer(int *dimensions, int *volume) {
      this->dims = dimensions;

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
        ->vert(-hd[0], -hd[1],  hd[2])->vert( hd[0], -hd[1],  hd[2])->vert( hd[0],  hd[1],  hd[2])
        ->vert(-hd[0],  hd[1],  hd[2])->vert( hd[0],  hd[1],  hd[2])->vert( hd[0],  hd[1], -hd[2])
        ->vert( hd[0], -hd[1], -hd[2])->vert( hd[0], -hd[1],  hd[2])->vert(-hd[0], -hd[1], -hd[2])
        ->vert( hd[0], -hd[1], -hd[2])->vert( hd[0],  hd[1], -hd[2])->vert(-hd[0],  hd[1], -hd[2])
        ->vert(-hd[0], -hd[1], -hd[2])->vert(-hd[0], -hd[1],  hd[2])->vert(-hd[0],  hd[1],  hd[2])
        ->vert(-hd[0],  hd[1], -hd[2])->vert( hd[0],  hd[1],  hd[2])->vert(-hd[0],  hd[1],  hd[2])
        ->vert(-hd[0],  hd[1], -hd[2])->vert( hd[0],  hd[1], -hd[2])->vert(-hd[0], -hd[1], -hd[2])
        ->vert( hd[0], -hd[1], -hd[2])->vert( hd[0], -hd[1],  hd[2])->vert(-hd[0], -hd[1],  hd[2])
        ->upload();

      // convert the volume into a 3d texture
      size_t size = dimensions[0] * dimensions[1] * dimensions[2];
      size_t textureSize = size * 3;
      this->volume = (GLbyte *)malloc(textureSize * sizeof(GLbyte));
      memset(this->volume, 0, size);
      for (size_t i=0; i<size; i++) {
        int val = volume[i];
        this->volume[i*3+0] = val;
        this->volume[i*3+1] = val;
        this->volume[i*3+2] = val;
      }

      glGenTextures(1, &this->volumeTexture);
      gl_error();
      glBindTexture(GL_TEXTURE_3D, this->volumeTexture);
      gl_error();

      glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
      glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
      glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
      glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);

      glTexImage3D(
        GL_TEXTURE_3D,
        0,
        GL_RGB,
        this->dims[0],
        this->dims[1],
        this->dims[2],
        0,
        GL_RGB,
        GL_UNSIGNED_BYTE,
        this->volume
      );

      gl_error();
    }

    ~Raytracer() {
      delete this->mesh;
      // free(this->volume);
    }

    void render (glm::mat4 mvp, glm::vec3 eye) {
      this->program->use();
      this->program
          ->uniformMat4("MVP", mvp)
          ->uniformVec3("eye", eye)
          ->uniformVec3i("dims", this->dims);

      glBindTexture(GL_TEXTURE_3D, this->volumeTexture);
      glActiveTexture(GL_TEXTURE0);
      gl_error();

      this->program->uniform1i("volume", 0);

      this->mesh->render(this->program, "position");
    }
  };

#endif
