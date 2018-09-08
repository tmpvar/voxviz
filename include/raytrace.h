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

  #include "brick.h"
  #include "core.h"

  using namespace std;

  class Raytracer {
    public:
    Program *program;
    int showHeat;

    Raytracer(int *dimensions) {
      this->showHeat = 0;
     
      this->program = new Program();
      this->program
          ->add(Shaders::get("raytrace.vert"))
          ->add(Shaders::get("raytrace.frag"))
          ->output("outColor")
          ->link();

      this->program->use();
    }

    ~Raytracer() {
    
    }
    
    void render(Volume *volume, Program *p) {
      glDrawElementsInstanced(
        GL_TRIANGLES,
        volume->mesh->faces.size(),
        GL_UNSIGNED_INT,
        0,
        volume->bricks.size()
      );
       gl_error();
    }
  };

#endif
