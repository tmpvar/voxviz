#pragma once

#include <string>
#include <iostream>
#include <vector>
#include <unordered_map>
#include <filesystem>

#include <json.hpp>
#include <glm/glm.hpp>
#include "uv.h"

#include "flatbuffers/flatbuffers.h"

using namespace std;
using namespace glm;

namespace renderpipe {
  void renderpipe_process_exit(uv_process_t*, int64_t exit_status, int term_signal);
  void alloc_buffer(uv_handle_t *handle, size_t len, uv_buf_t *buf);
  void read_stdout(uv_stream_t* stream, ssize_t nread, const uv_buf_t* buf);

  class Buffer {
      uvec3 dims;
      flatbuffers::FlatBufferBuilder builder;
    public:
      Buffer() {}
  };

  class Pipeline {
    vector<Program *> programs;
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
      unordered_map<string, Buffer> buffers;
      unordered_map<string, Pipeline> pipelines;
      string cwd;

      RenderPipe() {
        TCHAR NPath[MAX_PATH];
        GetCurrentDirectory(MAX_PATH, NPath);
        this->cwd = string(NPath);

        uv_pipe_init(uv_default_loop(), &this->process_stdout_pipe, 0);

      }

      string filename = "";
      string pipelineString = "";
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
        //uv_pipe_open(&this->process_stdout_pipe, 0);

        //child_stdio[1].flags = uv_stdio_flags::UV_INHERIT_FD;
        //child_stdio[1].data.fd = 1;
        child_stdio[1].flags = (uv_stdio_flags)(UV_CREATE_PIPE | UV_WRITABLE_PIPE);
        //child_stdio[1].flags = UV_CREATE_PIPE;
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
        this->process_options.args = args;


        this->pipelineString = "";

        if (uv_spawn(uv_default_loop(), &this->process_req, &this->process_options)) {
          cout << "spawn command failed" << endl;
          return false;
        }

        uv_read_start((uv_stream_t*)&this->process_stdout_pipe, alloc_buffer, read_stdout);

        free(targetChar);
        return true;
      }

      void collectPipelineString(char *str, ssize_t len) {
        this->pipelineString.append(str, len);
      }

      void generatePipelinesComplete(bool success) {
        cout << "pipeline complete: " << success << endl;
        cout << "pipeline contents:" << endl << this->pipelineString << endl;
        uv_close((uv_handle_t *)&this->process_stdout_pipe, NULL);
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
      RenderPipe::instance().collectPipelineString(buf->base, nread);
    }

    if (buf->base) {
      free(buf->base);
    }
  }

}