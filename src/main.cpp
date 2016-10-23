#define GLFW_INCLUDE_GLCOREARB

#include <GLFW/glfw3.h>
#include <stdio.h>

#include "vec.h"
#include "orbit-camera.h"
#include "mesher.h"

// Shader sources
const GLchar* vertexSource =
  "#version 150 core\n"
  "in vec3 position;"
  "uniform mat4 MVP;"
  "uniform ivec3 dims;"
  "out vec4 color;"
  "void main()"
  "{"
  "  color = vec4(position/dims, 1.0);"
  "  gl_Position = MVP * vec4(position, 1.0);"
  "}";

const GLchar* fragmentSource =
  "#version 150 core\n"
  "in vec4 color;"
  "out vec4 outColor;"
  "void main()"
  "{"
  // "  outColor = vec4(1.0);"
  "  outColor = color;"
  "}";

/*
  https://github.com/nothings/stb/blob/master/stb_voxel_render.h
*/

#include <iostream>
#include <math.h>
#include <string.h>
#include <math.h>

#define vol(i, j, k) volume[i + dims[0] * (j + dims[1] * k)]

bool keys[1024];
mat4 viewMatrix, perspectiveMatrix, MVP;

void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods) {
  if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS) {
    glfwSetWindowShouldClose(window, GLFW_TRUE);
  }

  if (action == GLFW_PRESS) {
    keys[key] = true;
  }

  if (action == GLFW_RELEASE) {
    keys[key] = false;
  }
}

float window_aspect(GLFWwindow *window) {
  int width, height;
  glfwGetWindowSize(window, &width, &height);

  float fw = static_cast<float>(width);
  float fh = static_cast<float>(height);

  return fabs(fw / fh);
}

void window_size_callback(GLFWwindow* window, int width, int height) {
  // When reshape is called, update the view and projection matricies since this means the view orientation or size changed
  mat4_perspective(
    perspectiveMatrix,
    65.0f * (M_PI / 180.0f),
    window_aspect(window),
    0.1f,
    1000.0f
  );

  glViewport(0, 0, width, height);
}

int main(void) {
  memset(keys, 0, sizeof(keys));

  int d = 32;
  int hd = d / 2;
  int dims[3] = { d, d, d };
  size_t total_voxels = dims[0] * dims[1] * dims[2];
  int *volume = (int *)malloc(total_voxels * sizeof(int));
  memset(volume, 0, total_voxels * sizeof(int));

  float camera_z = sqrt(d*d + d*d + d*d) * 1.5;

  for (int x=0; x<dims[0]; x++) {
    for (int y=0; y<dims[1]; y++) {
      for (int z=0; z<dims[2]; z++) {

        int dx = x - hd;
        int dy = y - hd;
        int dz = z - hd;

        vol(x, y, z) = sqrt(dx*dx + dy*dy + dz*dz) - (hd+5) > 0;
      }
    }
  }

  std::vector<float> out_verts;
  std::vector<unsigned int> out_faces;

  vx_mesher(volume, dims, out_verts, out_faces);

  GLfloat *vertices = (GLfloat *)out_verts.data();
  GLuint *elements = (GLuint *)out_faces.data();

  std::cout << "verts: " << out_verts.size() << " elements: " << out_faces.size() << std::endl;

  GLFWwindow* window;

  if (!glfwInit()) {
    return -1;
  }

  glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
  glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 2);
  glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
  glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

  window = glfwCreateWindow(640, 480, "voxviz", NULL, NULL);
  glfwSetWindowSizeCallback(window, window_size_callback);

  if (!window) {
    glfwTerminate();
    return -1;
  }

  glfwSetKeyCallback(window, key_callback);
  glfwMakeContextCurrent(window);

  GLuint vao;
  glGenVertexArrays(1, &vao);
  glBindVertexArray(vao);

  GLuint vbo;
  glGenBuffers(1, &vbo);
  glBindBuffer(GL_ARRAY_BUFFER, vbo);
  glBufferData(GL_ARRAY_BUFFER, out_verts.size()*3*sizeof(GLfloat), vertices, GL_STATIC_DRAW);

  GLuint ebo;
  glGenBuffers(1, &ebo);
  glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo);
  glBufferData(GL_ELEMENT_ARRAY_BUFFER, out_faces.size()*3*sizeof(GLfloat), elements, GL_STATIC_DRAW);

  GLuint vertexShader = glCreateShader(GL_VERTEX_SHADER);
  glShaderSource(vertexShader, 1, &vertexSource, NULL);
  glCompileShader(vertexShader);

  GLuint fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
  glShaderSource(fragmentShader, 1, &fragmentSource, NULL);
  glCompileShader(fragmentShader);

  GLuint shaderProgram = glCreateProgram();
  glAttachShader(shaderProgram, vertexShader);
  glAttachShader(shaderProgram, fragmentShader);
  glBindFragDataLocation(shaderProgram, 0, "outColor");
  glLinkProgram(shaderProgram);
  glUseProgram(shaderProgram);

  GLint mvpUniform = glGetUniformLocation(shaderProgram, "MVP");
  GLint dimsUniform = glGetUniformLocation(shaderProgram, "dims");

  GLint posAttrib = glGetAttribLocation(shaderProgram, "position");
  glEnableVertexAttribArray(posAttrib);
  glVertexAttribPointer(posAttrib, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(GLfloat), 0);

  // Setup the orbit camera
  vec3 eye = vec3_create(0.0f, 0.0f, camera_z);
  vec3 center = vec3_create(dims[0]/2.0f, dims[1]/2.0f, dims[2]/2.0f);
  vec3 up = vec3_create(0.0, 1.0, 0.0 );

  orbit_camera_init(eye, center, up);

  mat4_perspective(
    perspectiveMatrix,
    65.0f * (M_PI / 180.0f),
    window_aspect(window),
    0.1f,
    1000.0f
  );

  glUniform3i(dimsUniform, dims[0], dims[1], dims[2]);

  while (!glfwWindowShouldClose(window)) {
    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LESS);

    glClearColor(0.0, 0.0, 0.0, 1.0);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    orbit_camera_rotate(0, 0, -.01, .01);

    orbit_camera_view((float *)&viewMatrix);
    mat4_mul(MVP, perspectiveMatrix, viewMatrix);
    glUniformMatrix4fv(mvpUniform, 1, GL_FALSE, &MVP[0]);
    glDrawElements(GL_TRIANGLES, out_verts.size() * 3, GL_UNSIGNED_INT, 0);

    glfwSwapBuffers(window);
    glfwPollEvents();
  }

  glDeleteProgram(shaderProgram);
  glDeleteShader(fragmentShader);
  glDeleteShader(vertexShader);

  glDeleteBuffers(1, &ebo);
  glDeleteBuffers(1, &vbo);

  glDeleteVertexArrays(1, &vao);

  glfwTerminate();
  return 0;
}
