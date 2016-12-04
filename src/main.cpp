#include "gl-wrap.h"
#include <stdio.h>
#include <glm/glm.hpp>

#include "orbit-camera.h"
#include "mesher.h"
#include "raytrace.h"
#include "compute.h"

#include <shaders/built.h>
#include <iostream>

#define _USE_MATH_DEFINES
#include <math.h>
#include <string.h>

bool keys[1024];
glm::mat4 viewMatrix, perspectiveMatrix, MVP;

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

float window_aspect(GLFWwindow *window, int *width, int *height) {
  glfwGetFramebufferSize(window, width, height);

  float fw = static_cast<float>(*width);
  float fh = static_cast<float>(*height);

  return fabs(fw / fh);
}

void window_resize(GLFWwindow* window, int a = 0, int b = 0) {
  int width, height;

  // When reshape is called, update the view and projection matricies since this means the view orientation or size changed

  perspectiveMatrix = glm::perspective(
    45.0f,
    window_aspect(window, &width, &height),
    0.1f,
    10000.0f
  );

  glViewport(0, 0, width, height);
}

int main(void) {
  memset(keys, 0, sizeof(keys));

  int d = 128;
  int hd = d / 2;
  int dims[3] = { d, d, d };
  size_t total_voxels = dims[0] * dims[1] * dims[2];
  float camera_z = sqrt(d*d + d*d + d*d) * 1.5;

  GLFWwindow* window;

  if (!glfwInit()) {
    return -1;
  }

  glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
  glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 2);
  glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
  glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
  glfwSwapInterval(0);

  window = glfwCreateWindow(640, 480, "voxviz", NULL, NULL);
  glfwSetWindowSizeCallback(window, window_resize);

  if (!window) {
    glfwTerminate();
    return -1;
  }

  glfwSetKeyCallback(window, key_callback);
  glfwMakeContextCurrent(window);
  gladLoadGLLoader((GLADloadproc)glfwGetProcAddress);

  Mesh *voxel_mesh = new Mesh();
  Compute *compute = new Compute();

  Shaders::init();

  Program *prog = new Program();
  prog->add(Shaders::get("basic.vert"))
      ->add(Shaders::get("color.frag"))
      ->output("outColor")
      ->link()
      ->use();

  GLint mvpUniform = glGetUniformLocation(prog->handle, "MVP");
  GLint dimsUniform = glGetUniformLocation(prog->handle, "dims");

  // Setup the orbit camera
  glm::vec3 center(256.0, 0.0, 0.0);
  glm::vec3 eye(0.0, 0.0, 200);//glm::length(center) * 2.0);
  
  glm::vec3 up(0.0, 1.0, 0.0);
  
  window_resize(window);

  Raytracer *raytracer = new Raytracer(dims, compute->job);
  orbit_camera_init(eye, center, up);

  int time = 0;
  while (!glfwWindowShouldClose(window)) {
    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LESS);

    glClearColor(0.0, 0.0, 0.0, 1.0);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    if (keys[GLFW_KEY_W]) {
      orbit_camera_zoom(-5);
    }

    if (keys[GLFW_KEY_S]) {
      orbit_camera_zoom(5);
    }

    if (keys[GLFW_KEY_H]) {
      raytracer->showHeat = 1;
    } else {
      raytracer->showHeat = 0;
    }

    if (keys[GLFW_KEY_LEFT]) {
      orbit_camera_rotate(0, 0,0.02, 0);
    }

    if (keys[GLFW_KEY_RIGHT]) {
      orbit_camera_rotate(0, 0, -0.02, 0);
    }

    if (keys[GLFW_KEY_UP]) {
      orbit_camera_rotate(0, 0, 0, -0.02);
    }

    if (keys[GLFW_KEY_DOWN]) {
      orbit_camera_rotate(0, 0, 0, 0.02);
    }

    viewMatrix = orbit_camera_view();
    MVP = perspectiveMatrix * viewMatrix;

    glm::mat4 invertedView = glm::inverse(viewMatrix);
    glm::vec3 currentEye(invertedView[3][0], invertedView[3][1], invertedView[3][2]);
    raytracer->render(MVP, currentEye);

    glfwSwapBuffers(window);
    
    time++;
    for (int i = 0; i<VOLUME_COUNT; i++) {
      compute->fill(raytracer->volumes[i]->computeBuffer, raytracer->volumes[i]->center, time);
    }
    glfwPollEvents();
  }

  delete prog;
  delete voxel_mesh;
  delete raytracer;

  Shaders::destroy();

  glfwTerminate();
  return 0;
}
