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
  
  GLint gl_ok(GLint error);
  GLint gl_error();
  void gl_shader_log(GLuint shader);
  void gl_program_log(GLuint handle);

  static const char *shader_type(GLuint type) {
    switch (type) {
#ifdef GL_COMPUTE_SHADER
      case GL_COMPUTE_SHADER:
         return "GL_COMPUTE_SHADER";
      break;
#endif
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

    Shader(const char *src, const char *name, const GLuint type) {
      // this->src = src;
      this->type = type;
      this->handle = glCreateShader(type);
      glShaderSource(this->handle, 1, &src, NULL);

      std::cout << "Compile "
                << shader_type(type)
                << " Shader: "
                << name
                << std::endl;

      glCompileShader(this->handle);
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
      } else {
        ret = this->uniforms[name];
      }
      return ret;
    }

    Program *add(const Shader *shader) {
      glAttachShader(this->handle, shader->handle);
      return this;
    }

    Program *link() {
      glLinkProgram(this->handle);
      std::cout << "linking" << std::endl;
      gl_program_log(this->handle);

      return this;
    }

    Program *use() {
      glUseProgram(this->handle);
      this->texture_index = 0;
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
      glActiveTexture(GL_TEXTURE0 + this->texture_index);
      glBindTexture(GL_TEXTURE_2D, texture_id);
      this->use()->uniform1i(name, texture_index);
      this->texture_index++;
      return this;
    }
  };


  class Mesh {
  private:
    GLuint vao;
    GLuint vbo;
    GLuint ebo;
  public:
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
      glEnableVertexAttribArray(0);
      glVertexAttribPointer(
        0,
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
      program->use();
      program->attribute(attribute);
      glEnableVertexAttribArray(0);

      glBindVertexArray(this->vao);

      glDrawElements(
        GL_TRIANGLES,
        (GLsizei)this->faces.size(),
        GL_UNSIGNED_INT,
        0
      );
    }
  };


  class FBO {
  public:
    GLuint fb;
    unsigned int width;
    unsigned int height;

    GLuint texture_color;
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
      //}

      return this;
    }
    
    void create() {
      glGenFramebuffers(1, &this->fb);

      this->bind();
      this->attachColorTexture();
      this->attachDepthTexture();
      
      GLenum buffers[2] = { GL_COLOR_ATTACHMENT0, GL_DEPTH_ATTACHMENT };
      glDrawBuffers(2, buffers);

      this->status();
      this->unbind();
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
      glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, this->width, this->height, 0, GL_BGRA, GL_UNSIGNED_BYTE, NULL);

      glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, texture_color, 0/*mipmap level*/);
      this->status();
      return this;
    }

    FBO *attachDepthTexture() {
      // depth texture
      glGenTextures(1, &this->texture_depth);
      glBindTexture(GL_TEXTURE_2D, this->texture_depth);
      glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT32F, this->width, this->height, 0, GL_DEPTH_COMPONENT, GL_FLOAT, 0);
      glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
      glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
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
