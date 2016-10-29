#ifndef GL_WRAP_H
#define GL_WRAP_H
  #define GLFW_INCLUDE_GLCOREARB
  #include <GLFW/glfw3.h>
  #include <iostream>
  #include <vector>

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

    // Program *uniform(const char *name, )
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

  // ====
  /*
  Program *prog = new Program();
  prog->add(Shaders::get('basic.vert'))
      ->add(Shaders::get('color.frag'))
      ->output("outColor")
      ->link();


  prog->uniform("mvp", MVP)
  prog->input()

  // render mesh
  mesh->render(prog); // calls prog->use
  */
#endif
