#include <GLFW/glfw3.h>
#include <stdio.h>

#include "vec.h"
#include "orbit-camera.h"

// Shader sources
const GLchar* vertexSource =
  "#version 150 core\n"
  "in vec2 position;"
  "in vec3 color;"
  "uniform mat4 MVP;"
  "out vec3 Color;"
  "void main()"
  "{"
  "  Color = color;"
  "  gl_Position = MVP * vec4(position, 0.0, 1.0);"
  "}";
const GLchar* fragmentSource =
  "#version 150 core\n"
  "in vec3 Color;"
  "out vec4 outColor;"
  "void main()"
  "{"
  "  outColor = vec4(Color, 1.0);"
  "}";

/*

  if (_keys[NSRightArrowFunctionKey]) {
    orbit_camera_rotate(0, 0, .1, 0);
  }

  if (_keys[NSLeftArrowFunctionKey]) {
    orbit_camera_rotate(0, 0, -.1, 0);
  }


  if (_keys[NSUpArrowFunctionKey]) {
    orbit_camera_rotate(0, 0, 0, .1);
  }

  if (_keys[NSDownArrowFunctionKey]) {
    orbit_camera_rotate(0, 0, 0, -.1);
  }

  ---- reshape ----

  // When reshape is called, update the view and projection matricies since this means the view orientation or size changed
  float aspect = fabs(self.view.bounds.size.width / self.view.bounds.size.height);
  _projectionMatrix = matrix_from_perspective_fov_aspectLH(65.0f * (M_PI / 180.0f), aspect, 0.1f, 25.0f);

  _viewMatrix = matrix_from_translation(0.0f, -2.f, 14.0f);

*/




int main(void) {
  GLFWwindow* window;

  /* Initialize the library */
  if (!glfwInit())
    return -1;

  glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
  glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 2);
  glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
  glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

  /* Create a windowed mode window and its OpenGL context */
  window = glfwCreateWindow(640, 480, "Hello World", NULL, NULL);
  if (!window)
  {
    glfwTerminate();
    return -1;
  }

  /* Make the window's context current */
  glfwMakeContextCurrent(window);

  // Create Vertex Array Object
  GLuint vao;
  glGenVertexArrays(1, &vao);
  glBindVertexArray(vao);

  // Create a Vertex Buffer Object and copy the vertex data to it
  GLuint vbo;
  glGenBuffers(1, &vbo);

  GLfloat vertices[] = {
    -0.5f,  0.5f, 1.0f, 0.0f, 0.0f, // Top-left
     0.5f,  0.5f, 0.0f, 1.0f, 0.0f, // Top-right
     0.5f, -0.5f, 0.0f, 0.0f, 1.0f, // Bottom-right
    -0.5f, -0.5f, 1.0f, 1.0f, 1.0f  // Bottom-left
  };

  glBindBuffer(GL_ARRAY_BUFFER, vbo);
  glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

  // Create an element array
  GLuint ebo;
  glGenBuffers(1, &ebo);

  GLuint elements[] = {
    0, 1, 2,
    2, 3, 0
  };

  glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo);
  glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(elements), elements, GL_STATIC_DRAW);

  // Create and compile the vertex shader
  GLuint vertexShader = glCreateShader(GL_VERTEX_SHADER);
  glShaderSource(vertexShader, 1, &vertexSource, NULL);
  glCompileShader(vertexShader);

  // Create and compile the fragment shader
  GLuint fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
  glShaderSource(fragmentShader, 1, &fragmentSource, NULL);
  glCompileShader(fragmentShader);

  // Link the vertex and fragment shader into a shader program
  GLuint shaderProgram = glCreateProgram();
  glAttachShader(shaderProgram, vertexShader);
  glAttachShader(shaderProgram, fragmentShader);
  glBindFragDataLocation(shaderProgram, 0, "outColor");
  glLinkProgram(shaderProgram);
  glUseProgram(shaderProgram);

  GLint mvpUniform = glGetUniformLocation(shaderProgram, "MVP");


  // Specify the layout of the vertex data
  GLint posAttrib = glGetAttribLocation(shaderProgram, "position");
  glEnableVertexAttribArray(posAttrib);
  glVertexAttribPointer(posAttrib, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(GLfloat), 0);

  GLint colAttrib = glGetAttribLocation(shaderProgram, "color");
  glEnableVertexAttribArray(colAttrib);
  glVertexAttribPointer(colAttrib, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(GLfloat), (void*)(2 * sizeof(GLfloat)));

  // Setup the orbit camera
  vec3 eye = vec3_create(0.0f, 0.0f, 2.5);
  vec3 center = vec3f(0.0f);
  vec3 up = vec3_create(0.0, 1.0, 0.0 );

  orbit_camera_init(eye, center, up);

  mat4 base_model, perspectiveMatrix, MVP;

  float aspect = fabs(640.0 / 480.0);
  mat4_perspective(perspectiveMatrix, 65.0f * (M_PI / 180.0f), aspect, 0.1f, 25.0f);

  /* Loop until the user closes the window */
  while (!glfwWindowShouldClose(window))
  {
    glClearColor(1.0, 0.0, 1.0, 1.0);
    /* Render here */
    glClear(GL_COLOR_BUFFER_BIT);

    orbit_camera_rotate(0, 0, -.01, 0);

    orbit_camera_view((float *)&base_model);
    // mat4 modelViewMatrix = matrix_multiply(_viewMatrix, base_model);
    mat4_mul(MVP, perspectiveMatrix, base_model);

    glUniformMatrix4fv(mvpUniform, 1, GL_FALSE, &MVP[0]);

    // Draw a rectangle from the 2 triangles using 6 indices
    glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);

    /* Swap front and back buffers */
    glfwSwapBuffers(window);

    /* Poll for and process events */
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
