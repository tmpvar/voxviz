#pragma once

#ifdef __APPLE__
#define GLFW_INCLUDE_GLCOREARB
#else
#define GL_GLEXT_PROTOTYPES
#endif

#include "core.h"
#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <iostream>
#include <vector>
#include <string>
#include <map>
#include <utility>
#include <string>
#include <fstream>
#include <streambuf>
#include <sstream>
#include <imgui.h>

#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>

#ifdef __WIN32
  #ifndef max
  #define max(a,b)            (((a) > (b)) ? (a) : (b))
  #endif
#else
void DebugBreak();
#endif


using namespace std;

#ifndef DISABLE_GL_ERROR
#define gl_error() if (GL_ERROR()) { printf(" at " __FILE__ ":%d\n",__LINE__); DebugBreak(); exit(1);}
#else
#define gl_error() do {} while(0)
#endif

static map<string, string> shaderLogs;

GLint gl_ok(GLint error);
GLint GL_ERROR();
void gl_shader_log(GLuint shader);
void gl_program_log(GLuint handle);
void APIENTRY openglCallbackFunction(
  GLenum source,
  GLenum type,
  GLuint id,
  GLenum severity,
  GLsizei length,
  const GLchar* message,
  const void* userParam
);

static const char *shader_type(GLuint type) {
  switch (type) {
  case GL_COMPUTE_SHADER:
    return "GL_COMPUTE_SHADER";
    break;
  case GL_VERTEX_SHADER:
    return "GL_VERTEX_SHADER";
    break;
  case GL_TESS_CONTROL_SHADER:
    return "GL_TESS_CONTROL_SHADER";
    break;
  case GL_TESS_EVALUATION_SHADER:
    return "GL_TESS_EVALUATION_SHADER";
    break;
  case GL_GEOMETRY_SHADER:
    return "GL_GEOMETRY_SHADER";
    break;
  case GL_FRAGMENT_SHADER:
    return "GL_FRAGMENT_SHADER";
    break;
  }
  return "Unknown";
}

struct ShaderFileLine {
  string file;
  size_t line;
  size_t lineCount;
  size_t skipLines;
};

class Shader {
  bool valid = false;
public:
  GLuint handle;
  GLuint type;
  string name;
  string filename;
  size_t version;

  Shader(const char *src, const char *filename, const char *name, const GLuint type) {
    this->version = 1;
    this->filename = filename;
    this->type = type;
    this->name = name;
    this->reload();
  }

  bool isValid() {
    return this->valid;
  }

  bool reload() {
    this->valid = true;
    std::cout << "reloading shader: "
      << this->name
      << " (" << this->filename << ")"
      << std::endl;

    std::ifstream t(this->filename);
    std::string str;

    t.seekg(0, std::ios::end);
    str.reserve(t.tellg());
    t.seekg(0, std::ios::beg);

    str.assign((std::istreambuf_iterator<char>(t)),

      std::istreambuf_iterator<char>());
    const GLchar *source = (const GLchar *)str.c_str();
    GLuint new_handle = glCreateShader(this->type);
    glShaderSource(new_handle, 1, &source, NULL);
    std::cout
      << "Compile "
      << shader_type(type)
      << " Shader: "
      << name
      << std::endl;

    glCompileShader(new_handle);
    gl_ok(glGetError());

    GLint isCompiled = 0;
    glGetShaderiv(new_handle, GL_COMPILE_STATUS, &isCompiled);

    if (!isCompiled) {
      GLint l, m;
      glGetShaderiv(new_handle, GL_INFO_LOG_LENGTH, &m);
      char *s = (char *)malloc(m * sizeof(char));
      glGetShaderInfoLog(new_handle, m, &l, s);
      cout << this->name << "failed to compile" << endl
        << s << endl;

      // compute the line the error occurred on
      std::stringstream errorLogStream(s);
      stringstream sourceStream(str);
      string findFile = "// start: ";
      string errorLine;
      stringstream fileLog;
      while (std::getline(errorLogStream, errorLine, '\n')) {
        string sourceLine;
        string currentFile = "";
        size_t fileStartLine = 0;
        size_t currentLine = 1;
        size_t absoluteSourceLine = 1;

        size_t rawErrorLine = 0;

        size_t startingParenLoc = errorLine.find("(");
        if (startingParenLoc == string::npos) {
          cout << "could not parse gl error line: " << errorLine << endl;
          continue;
        }
        startingParenLoc += 1;

        // here we assume that errors are of the form 0(<line>) <message>
        size_t endingParenLoc = errorLine.substr(startingParenLoc).find(")");
        if (endingParenLoc == string::npos) {
          cout << "could not parse gl error line: " << errorLine << endl;
          continue;
        }

        rawErrorLine = stoul(errorLine.substr(startingParenLoc, endingParenLoc));

        cout << "error line: " << rawErrorLine << endl;
        sourceStream.seekg(0);

        vector<ShaderFileLine> file_stack;

        while (std::getline(sourceStream, sourceLine, '\n')) {
          absoluteSourceLine++;
          size_t fileCommentLoc = sourceLine.find(findFile);
          if (fileCommentLoc != string::npos) {
            currentFile = sourceLine.substr(fileCommentLoc + findFile.length());
            ShaderFileLine l;
            l.file = currentFile;
            l.line = absoluteSourceLine;
            l.lineCount = 0;
            l.skipLines = 0;
            file_stack.push_back(l);
          }

          if (absoluteSourceLine == rawErrorLine) {
            fileLog << absoluteSourceLine - (file_stack.back().line + file_stack.back().skipLines) << " " << sourceLine << endl;
            fileLog << "^ " << errorLine.substr(5 + endingParenLoc) << endl;
          }

          if (absoluteSourceLine == rawErrorLine + 1) {
            fileLog << absoluteSourceLine - (file_stack.back().line + file_stack.back().skipLines) << " " << sourceLine << endl;
            break;
          }

          if (sourceLine.find("// end: " + file_stack.back().file) != string::npos) {
            size_t skipLines = file_stack.back().lineCount + file_stack.back().skipLines;
            file_stack.pop_back();
            if (file_stack.size() == 0) {
              continue;
            }
            file_stack.back().skipLines += skipLines;

            if (absoluteSourceLine == rawErrorLine - 1) {
              fileLog << absoluteSourceLine - (file_stack.back().line + file_stack.back().skipLines)
                << " #include \""
                << file_stack.back().file
                << "\"" << endl;
            }
          }
          else if (absoluteSourceLine == rawErrorLine - 1) {
            fileLog << absoluteSourceLine - (file_stack.back().line + file_stack.back().skipLines) << " " << sourceLine << endl;
          }
          file_stack.back().lineCount++;
        }
      }
      shaderLogs[this->name] = fileLog.str();
      free(s);
      gl_shader_log(new_handle);
      this->valid = false;
    }
    else {
      shaderLogs.erase(this->name);
    }

    if (glIsShader(this->handle)) {
      glDeleteShader(this->handle);
    }

    this->handle = new_handle;
    this->version++;
    return true;
  }

  ~Shader() {
    glDeleteShader(this->handle);
  }
};

// TODO: consider renaming this to MappableSlab or something less gl specific
class SSBO {
  GLsizeiptr total_bytes = 0;
  GLuint handle = 0;
  bool mapped = false;
  uvec3 _dims = uvec3(0);
  GLbitfield _flags = GL_STATIC_DRAW;
  bool _persistent = false;
public:

  enum MAP_TYPE {
    MAP_READ_ONLY = GL_READ_ONLY,
    MAP_READ_WRITE = GL_READ_WRITE,
    MAP_WRITE_ONLY = GL_WRITE_ONLY,
  };

  SSBO(uint64_t bytes, bool persistent = false) {
    this->_persistent = persistent;
    if (persistent) {
      this->_flags = GL_MAP_PERSISTENT_BIT | GL_MAP_WRITE_BIT | GL_MAP_COHERENT_BIT;
    }
    this->resize(bytes);
  }

  SSBO(uint64_t bytes, uvec3 dims, bool persistent = false) {
    this->_persistent = persistent;
    if (persistent) {
      this->_flags = GL_MAP_PERSISTENT_BIT | GL_MAP_WRITE_BIT | GL_MAP_COHERENT_BIT;
    }
    this->resize(bytes, dims);
  }

  SSBO *resize(uint64_t bytes, uvec3 dims) {
    if (glm::all(glm::equal(dims, this->_dims)) && this->total_bytes == bytes) {
      return this;
    }

    this->_dims = dims;
    this->resize(bytes);
    return this;
  }

  SSBO *resize(uint64_t bytes) {
    // TODO: what do we do if we're mapped?
    if (this->_dims.x > 0) {
      std::cout << "creating SSBO with " << bytes << " bytes; dims:" << this->_dims.x << ", " << this->_dims.y << ", " << this->_dims.z << endl;
    }
    else {
      std::cout << "creating SSBO with " << bytes << " bytes" << endl;
    }

    if (this->total_bytes != 0 && bytes != this->total_bytes && this->handle != 0) {
      glDeleteBuffers(1, &this->handle);
    }

    if (bytes == 0) {
      return this;
    }

    glGenBuffers(1, &this->handle); gl_error();
    this->bind();
    if (this->_persistent) {
      glBufferStorage(GL_SHADER_STORAGE_BUFFER, bytes, NULL, this->_flags);
    }
    else {
      glBufferData(GL_SHADER_STORAGE_BUFFER, bytes, NULL, this->_flags);
    }

    gl_error();
    this->unbind();

    this->total_bytes = bytes;
    return this;
  }

  void *beginMap(MAP_TYPE m = MAP_READ_WRITE) {
    if (this->total_bytes == 0) {
      return nullptr;
    }

    this->bind();
    void *out = glMapBuffer(GL_SHADER_STORAGE_BUFFER, m); gl_error();
    //this->unbind();
    this->mapped = true;
    return out;
  }

  void *beginMapPersistent() {
    if (this->total_bytes == 0) {
      return nullptr;
    }

    GLuint access = GL_MAP_WRITE_BIT | GL_MAP_PERSISTENT_BIT | GL_MAP_COHERENT_BIT | GL_MAP_UNSYNCHRONIZED_BIT;

    this->bind();
    void *out = glMapBufferRange(GL_SHADER_STORAGE_BUFFER, 0, this->total_bytes, access);
    gl_error();

    this->mapped = true;
    return out;
  }

  void endMap() {
    if (!this->mapped || this->handle == 0) {
      return;
    }
    this->bind();
    glUnmapBuffer(GL_SHADER_STORAGE_BUFFER); gl_error();
    //this->unbind();
    this->mapped = false;
  }

  GLuint bind(GLuint type = GL_SHADER_STORAGE_BUFFER) {
    if (this->handle != 0) {
      glBindBuffer(type, this->handle); gl_error();
    }
    return this->handle;
  }

  void unbind(GLuint type = GL_SHADER_STORAGE_BUFFER) {
    glBindBuffer(type, 0); gl_error();
  }

  size_t size() {
    return this->total_bytes;
  }

  uvec3 dims() {
    return this->_dims;
  }

  SSBO *clear(uint8_t val) {
    this->bind();
    glClearBufferData(
      GL_SHADER_STORAGE_BUFFER,
      GL_R8,
      GL_RED,
      GL_UNSIGNED_BYTE,
      &val
    );
    return this;
  }
};

struct SSBOBinding {
  GLint block_index;
};

class Timer {
  GLuint query;
  double elapsed_time;
  bool done = false;
  bool started = 0;
  string name;
public:

  Timer() {
    glGenQueries(1, &this->query);
  }

  ~Timer() {
    glDeleteQueries(1, &this->query);
  }

  Timer *begin(const char *name) {
    this->done = false;
    this->name = string(name);
    glBeginQuery(GL_TIME_ELAPSED, this->query);
    this->started = true;
    return this;
  }

  Timer *end() {
    glEndQuery(GL_TIME_ELAPSED);
    return this;
  }

  bool isComplete() {
    if (!this->started) {
      return true;
    }

    GLint isDone = 0;
    glGetQueryObjectiv(
      this->query,
      GL_QUERY_RESULT_AVAILABLE,
      &isDone
    );

    if (isDone == 1) {
      this->done = true;
      GLuint64 time;
      glGetQueryObjectui64v(
        query,
        GL_QUERY_RESULT,
        &time
      );

      this->elapsed_time = double(time) / 1000000.0;
    }

    return this->done;
  }

  Timer *waitUntilComplete() {
    if (this->started && !this->done) {
      while (!this->isComplete()) {}
    }
    return this;
  }

  Timer *debug() {
    if (this->done) {
      ImGui::Text("%s: %.3f.ms", this->name.c_str(), this->elapsed_time);
    }
    return this;
  }
};

class Program {
  map<string, GLint> uniforms;
  map<string, GLint> attributes;
  map<string, GLint> resource_indices;
  map<string, GLint> ssbos;
  map<Shader *, size_t> shader_versions;
  map<string, GLuint> outputs;
  GLuint texture_index;
  string compositeName;
  GLint local_layout[3];
  bool isCompute = false;
  bool valid = false;
public:
  GLuint handle;

  Program() {
    this->handle = glCreateProgram();
    this->valid = true;
  }

  static Program *New() {
    return new Program();
  }

  ~Program() {
    if (glIsProgram(this->handle)) {
      glDeleteProgram(this->handle);
    }
  }

  bool isValid() {
    return this->valid;
  }

  GLint uniformLocation(string name) {
    GLint ret;
    if (this->uniforms.find(name) == this->uniforms.end()) {
      ret = glGetUniformLocation(this->handle, name.c_str());

      if (ret == GL_INVALID_VALUE) {
        std::cout << "uniformLocation returned invalid enum for: " << name << " on " << this->compositeName << std::endl;
        return ret;
      }

      if (ret == GL_INVALID_OPERATION) {
        std::cout << "uniformLocation returned invalid operation for: " << name << " on " << this->compositeName << std::endl;
        return ret;
      }

      this->uniforms[name] = ret;
      gl_error();
    }
    else {
      ret = this->uniforms[name];
    }
    return ret;
  }

  GLint resourceIndex(string name, GLenum type) {
    GLint ret;
    if (this->resource_indices.find(name) == this->resource_indices.end()) {
      ret = glGetProgramResourceIndex(this->handle, type, name.c_str());
      if (ret == GL_INVALID_ENUM) {
        std::cout << "resourceIndex returned invalid enum for: " << name << std::endl;
        return ret;
      }

      if (ret == GL_INVALID_INDEX) {
        std::cout << "resourceIndex returned invalid index for: " << name << std::endl;
        return ret;
      }

      this->resource_indices[name] = ret;

      //gl_error();
    }
    else {
      ret = this->resource_indices[name];
    }
    return ret;
  }

  Program *add(Shader *shader) {
    if (shader == nullptr) {
      this->valid = false;
      return this;
    }
    this->shader_versions.insert(std::make_pair(shader, (size_t)shader->version));
    this->compositeName += shader->name + " ";

    if (shader->type == GL_COMPUTE_SHADER) {
      this->isCompute = true;
    }

    if (!shader->isValid() || !this->valid) {
      this->valid = false;
      return this;
    }

    glAttachShader(this->handle, shader->handle);
    gl_error();
    return this;
  }

  Program *link() {
    if (!this->valid) {
      return this;
    }

    std::cout << "linking " << this->compositeName << std::endl;
    glLinkProgram(this->handle);
    gl_program_log(this->handle);
    gl_error();

    if (this->isCompute) {
      glGetProgramiv(this->handle, GL_COMPUTE_WORK_GROUP_SIZE, &this->local_layout[0]);
      if (GL_ERROR()) {
        this->valid = false;
      }
    }
    return this;
  }

  void rebuild() {
    if (glIsProgram(this->handle)) {
      glDeleteProgram(this->handle);
    }

    this->resource_indices.clear();
    this->attributes.clear();
    this->outputs.clear();
    this->uniforms.clear();
    this->ssbos.clear();

    this->handle = glCreateProgram();
    this->compositeName = "";
    this->valid = true;

    for (auto const& v : this->shader_versions) {
      Shader *shader = v.first;
      this->add(shader);
    }

    for (auto const& o : this->outputs) {
      string name = o.first;
      GLuint location = o.second;
      this->output(name, location);
    }

    this->link();

    for (auto const& v : this->shader_versions) {
      Shader *shader = v.first;
      this->shader_versions[shader] = shader->version;
    }
  }

  Program *use() {
#ifdef SHADER_HOTRELOAD
    for (auto& it : this->shader_versions) {
      Shader *shader = it.first;
      if (shader == nullptr) {
        this->valid = false;
        return this;
      }
      size_t version = it.second;
      if (version != shader->version) {
        std::cout << "rebuild program before use: " << this->compositeName << std::endl;
        this->rebuild();

        break;
      }
    }
#endif

    if (!this->valid) {
      return this;
    }

    static int used = 0;
    glUseProgram(this->handle);
    this->texture_index = 0;
    used++;
    gl_error();
    return this;
  }

  Program *output(string name, const GLuint location = 0) {
    if (!this->valid) {
      return this;
    }

    if (this->outputs.find(name) == this->outputs.end()) {
      this->outputs.emplace(name, location);
    }
    glBindFragDataLocation(this->handle, location, name.c_str());
    return this;
  }

  Program *attribute(string name) {
    if (!this->valid) {
      return this;
    }

    GLint posAttrib;
    if (attributes.find(name) == this->attributes.end()) {
      posAttrib = glGetAttribLocation(this->handle, name.c_str());
      this->attributes[name] = posAttrib;
    }
    else {
      posAttrib = this->attributes[name];
    }
    glEnableVertexAttribArray(posAttrib);
    return this;
  }

  Program *uniformFloat(string name, float v) {
    if (!this->valid) {
      return this;
    }

    GLint loc = this->uniformLocation(name);
    glUniform1f(loc, v);
    return this;
  }

  Program *uniformVec2(string name, glm::vec2 v) {
    if (!this->valid) {
      return this;
    }

    GLint loc = this->uniformLocation(name);
    glUniform2f(loc, v[0], v[1]);
    return this;
  }

  Program *uniformVec2ui(string name, glm::uvec2 v) {
    if (!this->valid) {
      return this;
    }

    GLint loc = this->uniformLocation(name);
    glUniform2ui(loc, v[0], v[1]);
    return this;
  }

  Program *uniformVec3(string name, glm::vec3 v) {
    if (!this->valid) {
      return this;
    }

    GLint loc = this->uniformLocation(name);

    glUniform3f(loc, v[0], v[1], v[2]);
    return this;
  }

  Program *uniformVec3fArray(string name, glm::vec3 *v, size_t count) {
    if (!this->valid) {
      return this;
    }

    GLint loc = this->uniformLocation(name);
    GLfloat *buf = (GLfloat *)malloc(sizeof(GLfloat) * 3 * count);

    for (size_t i = 0; i < count; i++) {
      buf[i * 3] = v[i].x;
      buf[i * 3 + 1] = v[i].y;
      buf[i * 3 + 2] = v[i].z;
    }

    glUniform3fv(loc, count, &buf[0]);
    gl_error();
    free(buf);
    return this;
  }

  Program *uniformVec3i(string name, glm::ivec3 v) {
    if (!this->valid) {
      return this;
    }

    GLint loc = this->uniformLocation(name);
    glUniform3i(loc, v[0], v[1], v[2]);
    return this;
  }

  Program *uniformVec3ui(string name, glm::uvec3 v) {
    if (!this->valid) {
      return this;
    }

    GLint loc = this->uniformLocation(name);
    glUniform3ui(loc, v[0], v[1], v[2]);
    return this;
  }

  Program *uniformVec4(string name, glm::vec4 v) {
    if (!this->valid) {
      return this;
    }

    GLint loc = this->uniformLocation(name);
    glUniform4f(loc, v[0], v[1], v[2], v[3]);
    return this;
  }

  Program *uniformMat4(string name, glm::mat4 v) {
    if (!this->valid) {
      return this;
    }

    GLint loc = this->uniformLocation(name);
    glUniformMatrix4fv(loc, 1, GL_FALSE, &v[0][0]);
    return this;
  }

  Program *uniform1i(string name, int i) {
    if (!this->valid) {
      return this;
    }

    GLint loc = this->uniformLocation(name);
    glUniform1i(loc, i);
    return this;
  }

  Program *uniform1ui(string name, uint32_t ui) {
    if (!this->valid) {
      return this;
    }

    GLint loc = this->uniformLocation(name);
    glUniform1ui(loc, ui);
    return this;
  }

  Program *texture2d(string name, GLuint  texture_id) {
    if (!this->valid) {
      return this;
    }

    glActiveTexture(GL_TEXTURE0 + this->texture_index);
    glBindTexture(GL_TEXTURE_2D, texture_id);
    this->uniform1i(name, texture_index);
    this->texture_index++;
    return this;
  }

  Program *texture3d(string name, GLuint  texture_id) {
    if (!this->valid) {
      return this;
    }

    glActiveTexture(GL_TEXTURE0 + this->texture_index);
    glBindTexture(GL_TEXTURE_3D, texture_id);
    this->uniform1i(name, texture_index);
    this->texture_index++;
    return this;
  }

  Program *bufferAddress(string name, GLuint64 val) {
    if (!this->valid) {
      return this;
    }

    GLint loc = this->uniformLocation(name);
    glProgramUniformui64NV(this->handle, loc, val);
    return this;
  }

  Program *ssbo(string name, SSBO *ssbo) {
    if (!this->valid) {
      return this;
    }

    if (ssbo == nullptr) {
      cout << "unable to bind ssbo because it was null. name: " << name << endl;
      return this;
    }

    GLint ri = this->resourceIndex(name, GL_SHADER_STORAGE_BLOCK);

    if (ri == GL_INVALID_ENUM) {
      printf("SSBO binding failed, invalid enum '%s'\n", name.c_str());
      return this;
    }

    if (ri == GL_INVALID_INDEX) {
      printf("SSBO binding failed, could not find '%s'\n", name.c_str());
      return this;
    }

    GLint idx;
    if (this->ssbos.find(name) == this->ssbos.end()) {
      idx = this->ssbos.size();
      this->ssbos[name] = idx;
      // Rebind the buffer index to the order in which it was seen in.
      glShaderStorageBlockBinding(this->handle, ri, idx);
    }
    else {
      idx = this->ssbos[name];
    }

    GLuint ssbo_handle = ssbo->bind();
    if (ssbo_handle != 0) {
      glBindBufferBase(GL_SHADER_STORAGE_BUFFER, idx, ssbo_handle); gl_error();
    }
    ssbo->unbind();
    return this;
  }


  Program *compute(glm::uvec3 dims, SSBO *indirect = nullptr) {
    this->use();
    if (!this->valid) {
      return this;
    }

    glm::vec3 local(
      this->local_layout[0],
      this->local_layout[1],
      this->local_layout[2]
    );

    glm::uvec3 d = glm::max(
      glm::uvec3(
        glm::ceil(glm::vec3(dims) / local)
      ),
      glm::uvec3(1)
    );
    glDispatchCompute(d.x, d.y, d.z);

    gl_error();
    return this;
  }

  Program *timedCompute(const char *str, glm::uvec3 dims, SSBO *indirect = nullptr) {
    if (!this->valid) {
      return this;
    }

#ifdef DISABLE_DEBUG_GL_TIMED_COMPUTE
    return this->compute(dims);
#else
    GLuint query;
    GLuint64 elapsed_time;
    GLint done = 0;
    glGenQueries(1, &query);
    glBeginQuery(GL_TIME_ELAPSED, query);

    this->compute(dims);

    glEndQuery(GL_TIME_ELAPSED);
    while (!done) {
      glGetQueryObjectiv(query, GL_QUERY_RESULT_AVAILABLE, &done);
    }

    // get the query result
    glGetQueryObjectui64v(query, GL_QUERY_RESULT, &elapsed_time);
    ImGui::Text("%s: %.3f.ms", str, elapsed_time / 1000000.0);
    glDeleteQueries(1, &query);
    return this;
#endif
  }

  Program *timedIndirectCompute(const char *str, SSBO *indirect) {
    if (!this->valid || indirect == nullptr) {
      return this;
    }

    indirect->bind(GL_DISPATCH_INDIRECT_BUFFER);
#ifndef DISABLE_DEBUG_GL_TIMED_COMPUTE
    GLuint query;
    GLuint64 elapsed_time;
    GLint done = 0;
    glGenQueries(1, &query);
    glBeginQuery(GL_TIME_ELAPSED, query);
#endif

    glDispatchComputeIndirect(0);
    gl_error();

#ifndef DISABLE_DEBUG_GL_TIMED_COMPUTE
    glEndQuery(GL_TIME_ELAPSED);
    while (!done) {
      glGetQueryObjectiv(query, GL_QUERY_RESULT_AVAILABLE, &done);
    }

    // get the query result
    glGetQueryObjectui64v(query, GL_QUERY_RESULT, &elapsed_time);
    ImGui::Text("%s: %.3f.ms", str, elapsed_time / 1000000.0);
    glDeleteQueries(1, &query);
#endif
    return this;
  }
};

class GPUSlab {
  GLuint64 address = 0;
  uint32_t handle = 0;
  uint32_t total_bytes;
  bool mapped = false;

public:

  GPUSlab(uint32_t bytes) {
    cout << "create GPUSlab of " << bytes << " bytes" << endl;
    this->resize(bytes);
  }

  GLuint64 getAddress() {
    return this->address;
  }

  enum MAP_TYPE {
    MAP_READ_ONLY = GL_READ_ONLY,
    MAP_READ_WRITE = GL_READ_WRITE,
    MAP_WRITE_ONLY = GL_WRITE_ONLY,
  };

  GPUSlab *bind() {
    glBindBuffer(GL_TEXTURE_BUFFER, this->handle);
    gl_error();
    return this;
  }

  GPUSlab *unbind() {
    glBindBuffer(GL_TEXTURE_BUFFER, 0);
    gl_error();
    return this;
  }

  GPUSlab *resize(size_t bytes) {
    if (this->total_bytes != 0 && bytes != this->total_bytes && this->handle != 0) {
      this->bind();
      glMakeBufferNonResidentNV(GL_TEXTURE_BUFFER);
      glDeleteBuffers(1, &this->handle);
      this->unbind();
    }

    if (bytes == 0) {
      return this;
    }

    glGenBuffers(1, &this->handle);
    gl_error();

    this->bind();

    glBufferData(
      GL_TEXTURE_BUFFER,
      bytes,
      NULL,
      GL_STATIC_DRAW
    );
    gl_error();

    glMakeBufferResidentNV(GL_TEXTURE_BUFFER, GL_READ_WRITE);
    gl_error();

    glGetBufferParameterui64vNV(GL_TEXTURE_BUFFER, GL_BUFFER_GPU_ADDRESS_NV, &this->address);
    gl_error();

    this->total_bytes = bytes;
    return this;
  }

  void *beginMap(MAP_TYPE m = MAP_READ_WRITE) {
    if (this->total_bytes == 0) {
      return nullptr;
    }

    this->bind();
    void *out = glMapBuffer(GL_TEXTURE_BUFFER, m); gl_error();
    this->unbind();
    this->mapped = true;
    return out;
  }

  void endMap() {
    if (this->total_bytes == 0 || !this->mapped || this->handle == 0) {
      return;
    }
    this->bind();
    glUnmapBuffer(GL_TEXTURE_BUFFER); gl_error();
    this->unbind();
    this->mapped = false;
  }
};

class Mesh {
public:
  GLuint vao;
  GLuint vbo;
  GLuint ebo;
  std::vector<GLfloat> verts;
  std::vector<GLuint> faces;

  Mesh() {}

  ~Mesh() {
    glDeleteBuffers(1, &this->ebo);
    glDeleteBuffers(1, &this->vbo);
    glDeleteVertexArrays(1, &this->vao);
  }

  void upload() {
    std::cout << "upload" << std::endl;
    glGenVertexArrays(1, &this->vao);
    gl_error();

    glBindVertexArray(this->vao);
    gl_error();

    glGenBuffers(1, &this->vbo);
    gl_error();

    glBindBuffer(GL_ARRAY_BUFFER, this->vbo);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(
      0,
      3,
      GL_FLOAT,
      GL_FALSE,
      3 * sizeof(GLfloat),
      0
    );

    std::cout << "buffering " << this->verts.size() / 3 << " vertices" << std::endl;
    glBufferData(
      GL_ARRAY_BUFFER,
      this->verts.size() * sizeof(GLfloat),
      (GLfloat *)this->verts.data(),
      GL_STATIC_DRAW
    );

    glGenBuffers(1, &this->ebo);
    gl_error();
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, this->ebo);
    gl_error();


    std::cout << "buffering " << this->faces.size() / 3 << " faces" << std::endl;
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(
      1,
      3,
      GL_FLOAT,
      GL_FALSE,
      3 * sizeof(GLfloat),
      0
    );
    glBufferData(
      GL_ELEMENT_ARRAY_BUFFER,
      this->faces.size() * sizeof(GLuint),
      (GLuint *)this->faces.data(),
      GL_STATIC_DRAW
    );
    gl_error();

    std::cout << "vao: " << this->vao << " vbo: " << this->vbo << " ebo: " << this->ebo << std::endl;

  }

  Mesh* vert(float x, float y, float z) {
    this->verts.push_back(x);
    this->verts.push_back(y);
    this->verts.push_back(z);
    return this;
  }

  Mesh* face(GLuint a, GLuint b, GLuint c) {
    this->faces.push_back(a);
    this->faces.push_back(b);
    this->faces.push_back(c);
    return this;
  }

  Mesh* edge(GLuint a, GLuint b) {
    this->faces.push_back(a);
    this->faces.push_back(b);
    return this;
  }

  void render(Program *program, const char* attribute) {
    program->attribute(attribute);
    gl_error();
    glEnableVertexAttribArray(0);
    gl_error();
    glBindVertexArray(this->vao);
    gl_error();
    glDrawElements(
      GL_TRIANGLES,
      (GLsizei)this->faces.size(),
      GL_UNSIGNED_INT,
      0
    );
    gl_error();
  }
};


class FBO {
public:
  GLuint fb;
  unsigned int width;
  unsigned int height;

  GLuint texture_color;
  GLuint texture_position;
  GLuint texture_depth;
  FBO(unsigned int width, unsigned int height) {
    this->fb = 0;
    this->width = width;
    this->height = height;
    this->create();
  }

  ~FBO() {
    this->destroy();
  }

  FBO* setDimensions(unsigned int width, unsigned int height) {
    this->destroy();
    this->width = width;
    this->height = height;
    this->create();
    return this;
  }

  void destroy() {
    glDeleteTextures(1, &texture_color);
    glDeleteTextures(1, &texture_position);
    glDeleteTextures(1, &texture_depth);

    // ensure we return the main render target back to
    // the screen (aka 0)
    this->unbind();

    glDeleteFramebuffers(1, &this->fb);
  }

  FBO* bind() {
    // Avoid needless rebinding when already bound.
    //if (!this->isCurrentlyBound()) {
    glBindFramebuffer(GL_FRAMEBUFFER, this->fb);
    //}
    gl_error();

    return this;
  }

  bool isCurrentlyBound() {
    GLint current;
    glGetIntegerv(GL_FRAMEBUFFER_BINDING, &current);
    gl_ok(current);
    return current == this->fb;
  }

  FBO* unbind() {
    // If another fbo is bound don't unbind it.
    //if (this->isCurrentlyBound()) {
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    gl_error();
    //}

    return this;
  }

  void create() {
    glGenFramebuffers(1, &this->fb);
    gl_error();
    this->bind();
    this->attachColorTexture();
    gl_error();
    this->attachPositionTexture();
    gl_error();
    this->attachDepthTexture();
    gl_error();

    GLenum buffers[2] = { GL_COLOR_ATTACHMENT0, GL_COLOR_ATTACHMENT1 };
    glDrawBuffers(2, buffers);
    gl_error();

    this->status();
    //this->unbind();
  }

  FBO *status() {
    gl_ok(glCheckFramebufferStatus(GL_FRAMEBUFFER));
    return this;
  }

  FBO *attachColorTexture() {
    glGenTextures(1, &this->texture_color);
    glBindTexture(GL_TEXTURE_2D, this->texture_color);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexImage2D(
      GL_TEXTURE_2D,
      0,
      GL_RGBA8,
      this->width,
      this->height,
      0,
      GL_BGRA,
      GL_UNSIGNED_BYTE,
      NULL
    );

    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, texture_color, 0/*mipmap level*/);
    this->status();
    return this;
  }

  FBO *attachPositionTexture() {
    glGenTextures(1, &this->texture_position);
    glBindTexture(GL_TEXTURE_2D, this->texture_position);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexImage2D(
      GL_TEXTURE_2D,
      0,
      GL_RGB16F,
      this->width,
      this->height,
      0,
      GL_RGB,
      GL_FLOAT,
      NULL
    );



    glFramebufferTexture2D(
      GL_FRAMEBUFFER,
      GL_COLOR_ATTACHMENT1,
      GL_TEXTURE_2D,
      this->texture_position,
      0/*mipmap level*/
    );

    this->status();

    return this;
  }


  FBO *attachDepthTexture() {
    // depth texture
    glGenTextures(1, &this->texture_depth);
    glBindTexture(GL_TEXTURE_2D, this->texture_depth);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT32F, this->width, this->height, 0, GL_DEPTH_COMPONENT, GL_FLOAT, 0);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

    glFramebufferTexture(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, texture_depth, 0/*mipmap level*/);

    this->status();
    return this;
  }

  void debugRender(int dimensions[2]) {
    glBindFramebuffer(GL_READ_FRAMEBUFFER, this->fb);
    glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);
    glBlitFramebuffer(
      0,
      0,
      dimensions[0],
      dimensions[1],
      0,
      0,
      dimensions[0],
      dimensions[1],
      GL_COLOR_BUFFER_BIT,
      GL_LINEAR
    );
  }
};