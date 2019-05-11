#pragma once

#include <gl-wrap.h>
#include <core.h>
#include <shaders/built.h>

class ABuffer {
  
  Program *clearProgram;
  Program *sortProgram;
  Program *rasterize;
  Program *debugProgram;

  FullscreenSurface *surface;

  SSBO *index;
  SSBO *value;

  uint32_t width = 0;
  uint32_t height = 0;


  public:

    ABuffer(uint32_t width, uint32_t height) {
      this->index = new SSBO(0);
      this->value = new SSBO(0);

      this->surface = new FullscreenSurface();

      this->clearProgram = new Program();
      this->clearProgram
        ->add(Shaders::get("a-buffer-clear.comp"))
        ->link();
        
      this->sortProgram = new Program();
      this->sortProgram
        ->add(Shaders::get("a-buffer-sort.comp"))
        ->link();

      this->rasterize = new Program();
      this->rasterize
        ->add(Shaders::get("a-buffer-rasterize.vert"))
        ->add(Shaders::get("a-buffer-rasterize.frag"))
        ->output("outColor")
        ->link();

      this->debugProgram = new Program();
      this->debugProgram
        ->add(Shaders::get("basic.vert"))
        ->add(Shaders::get("a-buffer-debug.frag"))
        ->output("outColor")
        ->link();

      this->resize(width, height);
    }

    void resize(uint32_t width, uint32_t height) {
      if (width == this->width || height == this->height) {
        return;
      }

      this->index->resize(width * height * 4);
      this->value->resize(width * height * ABUFFER_MAX_DEPTH_COMPLEXITY * sizeof(ABufferCell));
      this->width = width;
      this->height = height;
    }

    void clear() {
      glMemoryBarrier(GL_ALL_BARRIER_BITS);
      this->clearProgram
        ->use()
        ->ssbo("aBufferIndexSlab", this->index, 1)
        ->timedCompute("clear a-buffer", glm::uvec3(
          this->width,
          this->height,
          1
        ));
    }

    void renderVolume(Volume *volume, glm::mat4 VP, glm::vec3 eye, float debug) {
      //glMemoryBarrier(GL_ALL_BARRIER_BITS);
      glm::mat4 volumeModel = volume->getModelMatrix();
      glm::vec4 invEye = glm::inverse(volumeModel) * glm::vec4(eye, 1.0);
      this->rasterize
        ->use()
        ->ssbo("aBufferIndexSlab", this->index,  1)
        ->ssbo("aBufferValueSlab", this->value,  2)
        ->uniformMat4("MVP", VP * volumeModel)
        ->uniformMat4("M", volumeModel)
        ->uniformVec3("invEye", glm::vec3(
          invEye.x / invEye.w,
          invEye.y / invEye.w,
          invEye.z / invEye.w
        ))
        ->uniformMat4("model", volumeModel)
        ->uniformVec3("eye", eye)
        ->uniformFloat("maxDistance", MAX_DISTANCE)
        ->uniformFloat("debug", debug)
        ->uniformVec2ui("dims", glm::uvec2(this->width, this->height))
        ->uniformVec4("material", volume->material);

      size_t activeBricks = volume->bind();

      glDrawElementsInstanced(
        GL_TRIANGLES,
        volume->mesh->faces.size(),
        GL_UNSIGNED_INT,
        0,
        activeBricks
      );
    }

    void sort() {
      glMemoryBarrier(GL_ALL_BARRIER_BITS);
      this->sortProgram
        ->use()
        ->ssbo("aBufferIndexSlab", this->index, 1)
        ->ssbo("aBufferValueSlab", this->value, 2)
        ->timedCompute("sort a-buffer", glm::uvec3(
          this->width,
          this->height,
          1
        ));
    }

    void debug() {
      glMemoryBarrier(GL_ALL_BARRIER_BITS);
      GLuint query;
      GLuint64 elapsed_time;
      GLint done = 0;
      glGenQueries(1, &query);
      glBeginQuery(GL_TIME_ELAPSED, query);

      
      this->debugProgram 
        ->use()
        ->ssbo("aBufferIndexSlab", this->index, 1)
        ->ssbo("aBufferValueSlab", this->value, 2)
        ->uniformFloat("maxDistance", MAX_DISTANCE)
        ->uniformVec2ui("dims", glm::uvec2(this->width, this->height));
        
      this->surface->render(this->debugProgram);

      glEndQuery(GL_TIME_ELAPSED);
      while (!done) {
        glGetQueryObjectiv(query, GL_QUERY_RESULT_AVAILABLE, &done);
      }

      // get the query result
      glGetQueryObjectui64v(query, GL_QUERY_RESULT, &elapsed_time);
      ImGui::Text("debug a-buffer: %.3f.ms", elapsed_time / 1000000.0);
    }


};