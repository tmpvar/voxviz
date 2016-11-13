#ifndef GL_WRAP_H
#define GL_WRAP_H
  #define GLFW_INCLUDE_GLCOREARB
  #include <GLFW/glfw3.h>
  #include <iostream>
  #include <vector>

  #include "vec.h"

  static const char *shader_type(GLuint type) {
    switch (type) {
      // case GL_COMPUTE_SHADER:
      //   return "GL_COMPUTE_SHADER";
      // break;
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

    Shader(const char *src, const GLuint type) {
      // this->src = src;
      this->type = type;
      this->handle = glCreateShader(type);
      glShaderSource(this->handle, 1, &src, NULL);
      glCompileShader(this->handle);
      std::cout << "Compile "
                << shader_type(type)
                << " Shader: "
                << glGetError()
                << std::endl;
    }

    ~Shader() {
      glDeleteShader(this->handle);
    }
  };

  class Program {
  public:
    GLuint handle;

    Program() {
      this->handle = glCreateProgram();
    }

    ~Program() {
      glDeleteProgram(this->handle);
    }

    Program *add(const Shader *shader) {
      glAttachShader(this->handle, shader->handle);
      return this;
    }

    Program *link() {
      glLinkProgram(this->handle);
      return this;
    }

    Program *use() {
      glUseProgram(this->handle);
      return this;
    }

    Program *output(const char *name, const GLuint location = 0) {
      glBindFragDataLocation(this->handle, location, name);
      return this;
    }

    Program *attribute(const char *name) {
      glUseProgram(this->handle);
      GLint posAttrib = glGetAttribLocation(this->handle, "position");
      glEnableVertexAttribArray(posAttrib);
      glVertexAttribPointer(posAttrib, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(GLfloat), 0);
      return this;
    }

    // template<class T> Program *uniform(const char *name, const T&) {}
    //   // TODO: error handling
    //   GLint loc = glGetUniformLocation(this->handle, name);
    //   return this;
    // }

    Program *uniformVec2(const char *name, vec2 v) {
      GLint loc = glGetUniformLocation(this->handle, name);
      glUniform2f(loc, v[0], v[1]);
      return this;
    }

    Program *uniformVec3(const char *name, vec3 v) {
      GLint loc = glGetUniformLocation(this->handle, name);
      glUniform3f(loc, v[0], v[1], v[2]);
      return this;
    }

    Program *uniformVec3i(const char *name, int v[3]) {
      GLint loc = glGetUniformLocation(this->handle, name);
      glUniform3i(loc, v[0], v[1], v[2]);
      return this;
    }

    Program *uniformVec4(const char *name, vec3 v) {
      GLint loc = glGetUniformLocation(this->handle, name);
      glUniform4f(loc, v[0], v[1], v[2], v[3]);
      return this;
    }

    Program *uniformMat4(const char *name, mat4 v) {
      GLint loc = glGetUniformLocation(this->handle, name);
      glUniformMatrix4fv(loc, 1, GL_FALSE, &v[0]);
      return this;
    }
  };

  class Mesh {
  public:
    std::vector<GLfloat> verts;
    std::vector<GLuint> faces;

    GLuint vao;
    GLuint vbo;
    GLuint ebo;

    Mesh() {}

    ~Mesh() {
      glDeleteBuffers(1, &this->ebo);
      glDeleteBuffers(1, &this->vbo);
      glDeleteVertexArrays(1, &this->vao);
    }

    void upload() {
      glGenVertexArrays(1, &this->vao);
      glBindVertexArray(this->vao);

      glGenBuffers(1, &this->vbo);
      glBindBuffer(GL_ARRAY_BUFFER, this->vbo);
      glBufferData(
        GL_ARRAY_BUFFER,
        this->verts.size()*3*sizeof(GLfloat),
        (GLfloat *)this->verts.data(),
        GL_STATIC_DRAW
      );

      glGenBuffers(1, &this->ebo);
      glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, this->ebo);
      glBufferData(
        GL_ELEMENT_ARRAY_BUFFER,
        this->faces.size()*3*sizeof(GLuint),
        (GLuint *)this->faces.data(),
        GL_STATIC_DRAW
      );
    }

    void render (Program *program) {
      program->use();
      glBindVertexArray(this->vao);
      glDrawElements(
        GL_TRIANGLES,
        verts.size() * 3,
        GL_UNSIGNED_INT,
        0
      );
    }
  };
#endif
