#ifndef GL_WRAP_H
#define GL_WRAP_H

  #ifdef __APPLE__
    #define GLFW_INCLUDE_GLCOREARB
  #else
    #define GL_GLEXT_PROTOTYPES
  #endif

  #include <glad/glad.h>
  #include <GLFW/glfw3.h>
  #include <iostream>
  #include <vector>
  #include <string>
  #include <map>
  #include <glm/glm.hpp>

  using namespace std;
  
#define gl_error() if (GL_ERROR()) { printf(" at " __FILE__ ":%d\n",__LINE__); exit(1);}

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

  class Shader {
  public:
    GLuint handle;
    GLuint type;
    string name;

    Shader(const char *src, const char *name, const GLuint type) {
      // this->src = src;
      this->type = type;
      this->handle = glCreateShader(type);
      gl_error();
      this->name = name;
      glShaderSource(this->handle, 1, &src, NULL);
      gl_error();
      std::cout << "Compile "
                << shader_type(type)
                << " Shader: "
                << name
                << std::endl;

      glCompileShader(this->handle);
      gl_error();
      gl_shader_log(this->handle);
    }

    ~Shader() {
      glDeleteShader(this->handle);
    }
  };

  class Program {
    map<string, GLint> uniforms;
    map<string, GLint> attributes;
    GLuint texture_index;
    string compositeName;
  public:
    GLuint handle;

    Program() {
      this->handle = glCreateProgram();
    }

    ~Program() {
      glDeleteProgram(this->handle);
    }

    GLint uniformLocation(string name) {
      GLint ret;
      if (this->uniforms.find(name) == this->uniforms.end()) {
        ret = glGetUniformLocation(this->handle, name.c_str());
        this->uniforms[name] = ret;
        gl_error();
      } else {
        ret = this->uniforms[name];
      }
      return ret;
    }

    Program *add(const Shader *shader) {
      this->compositeName += " " + shader->name;
      glAttachShader(this->handle, shader->handle);
      gl_error();
      return this;
    }

    Program *link() {
      std::cout << "linking" << this->compositeName << std::endl;
      glLinkProgram(this->handle);
      gl_program_log(this->handle);
      gl_error();
      return this;
    }

    Program *use() {
      glUseProgram(this->handle);
      this->texture_index = 0;
      gl_error();
      return this;
    }

    Program *output(string name, const GLuint location = 0) {
      glBindFragDataLocation(this->handle, location, name.c_str());
      return this;
    }

    Program *attribute(string name) {
      GLint posAttrib;
      glUseProgram(this->handle);
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
      GLint loc = this->uniformLocation(name);
      glUniform1f(loc, v);
      return this;
    }

    Program *uniformVec2(string name, glm::vec2 v) {
      GLint loc = this->uniformLocation(name);
      glUniform2f(loc, v[0], v[1]);
      return this;
    }

    Program *uniformVec3(string name, glm::vec3 v) {
      GLint loc = this->uniformLocation(name);

      glUniform3f(loc, v[0], v[1], v[2]);
      return this;
    }

    Program *uniformVec3i(string name, int v[3]) {
      GLint loc = this->uniformLocation(name);
      glUniform3i(loc, v[0], v[1], v[2]);
      return this;
    }

    Program *uniformVec3ui(string name, glm::uvec3 v) {
      GLint loc = this->uniformLocation(name);
      glUniform3ui(loc, v[0], v[1], v[2]);
      return this;
    }

    Program *uniformVec4(string name, glm::vec3 v) {
      GLint loc = this->uniformLocation(name);
      glUniform4f(loc, v[0], v[1], v[2], v[3]);
      return this;
    }

    Program *uniformMat4(string name, glm::mat4 v) {
      GLint loc = this->uniformLocation(name);
      glUniformMatrix4fv(loc, 1, GL_FALSE, &v[0][0]);
      return this;
    }

    Program *uniform1i(string name, int i) {
      GLint loc = this->uniformLocation(name);
      glUniform1i(loc, i);
      return this;
    }


    Program *texture2d(string name, GLuint  texture_id) {
      GLint loc = this->uniformLocation(name);
      if (loc == -1) {
        cout << "unable to bind 2d texture for uniform:" << name << endl;
        return this;
      }

      glActiveTexture(GL_TEXTURE0 + this->texture_index);
      glBindTexture(GL_TEXTURE_2D, texture_id);
      glUniform1i(loc, texture_index);
      this->texture_index++;
      return this;
    }

    Program *texture2dArray(string name, GLuint texture_id) {
      GLint loc = this->uniformLocation(name);
      if (loc == -1) {
        cout << "unable to bind 2d texture array for uniform:" << name << endl;
        return this;
      }

      glActiveTexture(GL_TEXTURE0 + this->texture_index);
      glBindTexture(GL_TEXTURE_2D_ARRAY, texture_id);
      glUniform1i(loc, texture_index);
      this->texture_index++;
      return this;
    }

    Program *texture3d(string name, GLuint texture_id) {
      GLint loc = this->uniformLocation(name);
      if (loc == -1) {
        cout << "unable to bind 3d texture for uniform:" << name << endl;
        return this;
      }

      glActiveTexture(GL_TEXTURE0 + this->texture_index);
      glBindTexture(GL_TEXTURE_3D, texture_id);
      glUniform1i(loc, texture_index);
      this->texture_index++;
      return this;
    }

    Program *bufferAddress(string name, GLuint64 val) {
      GLint loc = this->uniformLocation(name);
      glProgramUniformui64NV(this->handle, loc, val);
      return this;
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
        this->verts.size()*sizeof(GLfloat),
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
        this->faces.size()*sizeof(GLuint),
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

    void render (Program *program, const char* attribute) {
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
#endif
