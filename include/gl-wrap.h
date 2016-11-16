#ifndef GL_WRAP_H
#define GL_WRAP_H

#if defined(__APPLE__)
#define GLFW_INCLUDE_GLCOREARB
#else
#define GL_GLEXT_PROTOTYPES
#endif
  #include <glad/glad.h>
  #include <GLFW/glfw3.h>
  #include <iostream>
  #include <vector>
  #include <glm/glm.hpp>

  GLint gl_error() {
    GLint error = glGetError();

    switch (error) {
      case GL_INVALID_ENUM:
        printf("error (%i): GL_INVALID_ENUM: An unacceptable value is specified for an enumerated argument. The offending command is ignored and has no other side effect than to set the error flag.\n", error);
      break;

      case GL_INVALID_VALUE:
        printf("error (%i): GL_INVALID_VALUE: A numeric argument is out of range. The offending command is ignored and has no other side effect than to set the error flag.\n", error);
      break;

      case GL_INVALID_OPERATION:
        printf("error (%i): GL_INVALID_OPERATION: The specified operation is not allowed in the current state. The offending command is ignored and has no other side effect than to set the error flag.\n", error);
      break;

      // case GL_STACK_OVERFLOW:
      //   printf("error (%i): GL_STACK_OVERFLOW: This command would cause a stack overflow. The offending command is ignored and has no other side effect than to set the error flag.\n", error);
      // break;

      // case GL_STACK_UNDERFLOW:
      //   printf("error (%i): GL_STACK_UNDERFLOW: This command would cause a stack underflow. The offending command is ignored and has no other side effect than to set the error flag.\n", error);
      // break;

      case GL_OUT_OF_MEMORY:
        printf("error (%i): GL_OUT_OF_MEMORY: There is not enough memory left to execute the command. The state of the GL is undefined, except for the state of the error flags, after this error is recorded.\n", error);
      break;
    }
    return error;
  }

  void gl_shader_log(GLuint shader) {
    GLint error = glGetError();
    GLint l, m;
    GLint isCompiled = 0;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &isCompiled);
    if(error || isCompiled == GL_FALSE) {
      glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &m);
      char *s = (char *)malloc(m * sizeof(char));
      glGetShaderInfoLog(shader, m, &l, s);
      printf("shader log:\n%s\n", s);
      free(s);
    }
  }

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
      std::cout << "linking" << std::endl;
      gl_error();

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
      GLint posAttrib = glGetAttribLocation(this->handle, name);
      glEnableVertexAttribArray(posAttrib);
      return this;
    }

    Program *uniformVec2(const char *name, glm::vec2 v) {
      GLint loc = glGetUniformLocation(this->handle, name);
      glUniform2f(loc, v[0], v[1]);
      return this;
    }

    Program *uniformVec3(const char *name, glm::vec3 v) {
      GLint loc = glGetUniformLocation(this->handle, name);
		
      glUniform3f(loc, v[0], v[1], v[2]);
      return this;
    }

    Program *uniformVec3i(const char *name, int v[3]) {
      GLint loc = glGetUniformLocation(this->handle, name);
      glUniform3i(loc, v[0], v[1], v[2]);
      return this;
    }

    Program *uniformVec4(const char *name, glm::vec3 v) {
      GLint loc = glGetUniformLocation(this->handle, name);
      glUniform4f(loc, v[0], v[1], v[2], v[3]);
      return this;
    }

    Program *uniformMat4(const char *name, glm::mat4 v) {
      GLint loc = glGetUniformLocation(this->handle, name);
      glUniformMatrix4fv(loc, 1, GL_FALSE, &v[0][0]);
      return this;
    }

    Program *uniform1i(const char *name, int i) {
      GLint loc = glGetUniformLocation(this->handle, name);
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
        this->faces.size(),
        GL_UNSIGNED_INT,
        0
      );
    }
  };
#endif
