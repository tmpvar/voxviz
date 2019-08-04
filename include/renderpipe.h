#pragma once

#include <string>
#include <iostream>
#include <vector>
#include <unordered_map>
#include <unistd.h>

#include <glm/glm.hpp>
#include "uv.h"
#include "flatbuffers/flatbuffers.h"
#include "renderpipe_generated.h"

using namespace std;
using namespace glm;

namespace renderpipe {
  void renderpipe_process_exit(uv_process_t*, int64_t exit_status, int term_signal);
  void alloc_buffer(uv_handle_t *handle, size_t len, uv_buf_t *buf);
  void read_stdout(uv_stream_t* stream, ssize_t nread, const uv_buf_t* buf);

  class Invocation {
    public:
    Invocation() {}
    Program *program;
    uvec3 dims;
  };

  class Stage {
    public:
      Stage() {

      }
      vector<Invocation *> invocations;
  };

  /*
    TODO:
    - launch node for [file]
      - collect output
      - regenerate pipelines
    - filewatch [file]
  */
  class RenderPipe {
    private:
      vector<SSBO *> buffers;
      unordered_map<string, Program *> programs;
      unordered_map<string, Stage *> stages;
      string cwd;
      GLuint dummyvao;

      RenderPipe() {
        #ifndef MAX_PATH
        #define MAX_PATH 1024
        #endif
        #ifndef TCHAR
        #define TCHAR char
        #endif
        TCHAR NPath[MAX_PATH];

        #ifdef __APPLE__
          getcwd(NPath, MAX_PATH);
        #else
          GetCurrentDirectory(MAX_PATH, NPath);
        #endif

        this->cwd = string(NPath);

        uv_pipe_init(uv_default_loop(), &this->process_stdout_pipe, 0);

        glGenVertexArrays(1, &this->dummyvao);
      }

      string filename = "";
      basic_string<uint8_t> pipelineString;
      uv_process_t process_req;
      uv_process_options_t process_options;
      uv_pipe_t process_stdout_pipe;

    public:
      static RenderPipe &instance() {
        static RenderPipe obj;
        return obj;
      }

      void setFilename(string filename) {
        this->filename = filename;
        this->generatePipelines();
      }

      bool generatePipelines() {
        uv_stdio_container_t child_stdio[3];

        this->process_options.stdio = child_stdio;

        // stdin
        child_stdio[0].flags = uv_stdio_flags::UV_IGNORE;

        // stdout
        child_stdio[1].flags = (uv_stdio_flags)(UV_CREATE_PIPE | UV_WRITABLE_PIPE);
        child_stdio[1].data.stream = (uv_stream_t*)&this->process_stdout_pipe;

        // stderr
        child_stdio[2].flags = uv_stdio_flags::UV_INHERIT_FD;
        child_stdio[2].data.fd = 2;

        string target = this->cwd + "/" + this->filename;
        char *targetChar = (char *)malloc(target.size() + 1);
        memset(targetChar, 0, target.size() + 1);
        memcpy(targetChar, target.c_str(), target.size());

        char* args[3] = { "node", targetChar, NULL };

        cout << "loading pipeline: " << target << endl;

        this->process_options.stdio_count = 3;
        this->process_options.exit_cb = renderpipe_process_exit;
        this->process_options.file = args[0];
        this->process_options.args = (char**)args;


        this->pipelineString.clear();

        if (uv_spawn(uv_default_loop(), &this->process_req, &this->process_options)) {
          cout << "spawn command failed" << endl;
          return false;
        }

        uv_read_start((uv_stream_t*)&this->process_stdout_pipe, alloc_buffer, read_stdout);

        free(targetChar);
        return true;
      }

      void collectPipelineString(uint8_t *str, ssize_t len) {
        this->pipelineString.append(str, len);
      }

      void generatePipelinesComplete(bool success) {
        cout << "pipeline complete: " << success << endl;
        uv_close((uv_handle_t *)&this->process_stdout_pipe, NULL);

        if (success) {
          this->buildPipeline();
        }
      }

      void buildPipeline() {
        const uint8_t *data = (uint8_t *)this->pipelineString.data();
        auto verifier = flatbuffers::Verifier(
          data,
          this->pipelineString.length()
        );
        bool verified = proto::VerifySceneBuffer(verifier);

        if (!verified) {
          cerr << "unable to verify flatbuffer" << endl;
          exit(1);
        }

        const proto::Scene *scene = proto::GetScene(data);

        this->buildBuffers(scene);
        this->buildPrograms(scene);
        this->buildInvocations(scene);
      }

      void buildBuffers(const proto::Scene *scene) {
        if (scene == nullptr) {
          return;
        }

        auto &buffers = *scene->buffers();
        for (auto buffer : buffers) {
          this->buffers.push_back(new SSBO(buffer->length()));
        }
      }

      void buildPrograms(const proto::Scene *scene) {
        for (auto program : *scene->programs()) {
          Program *p = new Program();

          for (auto shader : *program->shaders()) {
            GLuint type = 0;

            proto::ShaderType inType = shader->type();

            switch (inType) {
            case proto::ShaderType::ShaderType_Fragment:
              type = GL_FRAGMENT_SHADER;
              break;
            case proto::ShaderType::ShaderType_Vertex:
              type = GL_VERTEX_SHADER;
              break;
            case proto::ShaderType::ShaderType_Compute:
              type = GL_COMPUTE_SHADER;
              break;
            default:
              cout << "unknown ShaderType, skipping" << endl;
              continue;
            }

            Shader *s = new Shader(
              string(shader->source()->c_str()),
              string(shader->filename()->c_str()),
              string(shader->hash()->c_str()),
              type
            );

            p->add(s);
          }

          // TODO: pass this in via flatbuffer!
          if (!p->isCompute()) {
            p->output("color");
          }

          p->link();
          if (p->isValid()) {
            cout << "program is valid" << endl;
            string programHash = (program->hash()->c_str());
            this->programs[programHash] = p;
          }
        }
      }

      void buildInvocations(const proto::Scene *scene) {
        if (scene == nullptr || scene->stages() == nullptr) {
          return;
        }
        for (auto stage : *scene->stages()) {
          Stage *s =  new Stage();
          for (auto invocation : *stage->invocations()) {
            Invocation *i = new Invocation();

            i->dims = uvec3(
              invocation->dims()->x(),
              invocation->dims()->y(),
              invocation->dims()->z()
            );

            string programHash(invocation->program()->c_str());
            if (this->programs.find(programHash) != this->programs.end()) {
              i->program = this->programs[programHash];
            } else {
              cout << "could not find program" << endl;
            }

            s->invocations.push_back(i);
          }

          this->stages[string(stage->name()->c_str())] = s;
        }
      }

      void run(string stageName) {
        if (this->stages.find(stageName) == this->stages.end()) {
          return;
        }

        Stage *s = this->stages[stageName];

        for (auto invocation : s->invocations) {
          invocation->program->use();
          glBindVertexArray(this->dummyvao);

          // TODO: pull these states from invocations via proto
          glEnable(GL_DEPTH_TEST);
          glDepthFunc(GL_LESS);
          glDepthMask(GL_TRUE);

          glCullFace(GL_BACK);
          glEnable(GL_CULL_FACE);

          glDrawArrays(
            GL_TRIANGLES,
            0,
            invocation->dims.x
          );
          gl_error();
        }
      }

      RenderPipe(RenderPipe const&) = delete;
      void operator=(RenderPipe const&) = delete;
  };

  void renderpipe_process_exit(uv_process_t*, int64_t exit_status, int term_signal) {
    cout << "process exited status: " << exit_status << " signal:" << term_signal << endl;
    RenderPipe::instance().generatePipelinesComplete(exit_status == 0 && term_signal == 0);
  }

  void alloc_buffer(uv_handle_t *handle, size_t suggested_size, uv_buf_t *buf) {
    *buf = uv_buf_init((char*)malloc(suggested_size), suggested_size);
  }

  void read_stdout(uv_stream_t *stream, ssize_t nread, const uv_buf_t *buf) {
    if (nread > 0) {
      RenderPipe::instance().collectPipelineString((uint8_t *)buf->base, nread);
    }

    if (buf->base != nullptr) {
      free(buf->base);
    }
  }

}
