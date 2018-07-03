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
  
  GLint gl_error();
  void gl_shader_log(GLuint shader);
  

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

    Shader(const char *src, const GLuint type) {
      // this->src = src;
      this->type = type;
      this->handle = glCreateShader(type);
      glShaderSource(this->handle, 1, &src, NULL);

      std::cout << "Compile "
                << shader_type(type)
                << " Shader: "
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
        cout << "miss: " << name << endl;
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
      gl_error();

      return this;
    }

    Program *use() {
      glUseProgram(this->handle);
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
#endif
