#include <stdio.h>
#include <glm/glm.hpp>

#include "camera-free.h"
#include "raytrace.h"
//#include "compute.h"
#include "core.h"
//#include "clu.h"
#include "fullscreen-surface.h"
#include "shadowmap.h"
#include "volume.h"
#include "volume-manager.h"

#include <shaders/built.h>
#include <iostream>

#include <uv.h>
#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_opengl3.h>
#include <q3.h>
#include <glm/glm.hpp>
#include "parser/vzd/vzd.h"
#include "parser/magicavoxel/vox.h"
#include "blue-noise.h"
#include "scene.h"

#define _USE_MATH_DEFINES
#include <math.h>
#include <sstream>
#include <string.h>
#include <queue>
#include <map>

#include "renderpipe.h"

bool keys[1024];
bool prevKeys[1024];
double mouse[2];
bool fullscreen = 0;
bool yes = true;
// int windowDimensions[2] = { 1024, 768 };
int windowDimensions[2] = { 1440, 900 };
glm::mat4 viewMatrix, perspectiveMatrix, MVP;
const float fov = 45.0f;
FBO *fbo = nullptr;

SSBO *raytraceOutput = nullptr;
SSBO *terminationOutput = nullptr;

void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods) {
  if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS) {
    glfwSetWindowShouldClose(window, GLFW_TRUE);
  }

  prevKeys[key] = keys[key];

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
    fov,
    window_aspect(window, &width, &height),
    0.1f,
    10000.0f
  );

  printf("window resize %ix%i\n", width, height);

  glViewport(0, 0, width, height);
  if (fbo != nullptr) {
    fbo->setDimensions(width, height);
  }

  if (raytraceOutput != nullptr) {
    uint64_t outputBytes =
      static_cast<uint64_t>(width) *
      static_cast<uint64_t>(height) * 16;

    raytraceOutput->resize(outputBytes);
  }

  if (terminationOutput != nullptr) {
    uint64_t terminationBytes =
      static_cast<uint64_t>(width) *
      static_cast<uint64_t>(height) * sizeof(RayTermination);
    terminationOutput->resize(terminationBytes);
  }
}

GLFWwindow* createWindow(ivec2 resolution) {
  GLFWwindow* window = nullptr;

  if (!glfwInit()) {
    return nullptr;
  }

  glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
  glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
  glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
  glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

  #ifndef FULLSCREEN
    window = glfwCreateWindow(resolution.x, resolution.y, "voxviz", NULL, NULL);
  #else
    // fullscreen
    GLFWmonitor* primary = glfwGetPrimaryMonitor();
    const GLFWvidmode* mode = glfwGetVideoMode(primary);
    window = glfwCreateWindow(mode->width, mode->height, "voxviz", glfwGetPrimaryMonitor(), NULL);
  #endif

  if (!window) {
    glfwTerminate();
    return nullptr;
  }

  // Setup window event callbacks
  glfwSetWindowSizeCallback(window, window_resize);
  glfwSetKeyCallback(window, key_callback);

  // FIXME: if we make more than one window (unlikely) we'll want to move
  //        the glad initialization
  glfwMakeContextCurrent(window);
  gladLoadGLLoader((GLADloadproc)glfwGetProcAddress);
  glfwSwapInterval(0);

  // Setup Dear ImGui binding
  IMGUI_CHECKVERSION();
  ImGui::CreateContext();
  ImGui_ImplGlfw_InitForOpenGL(window, false);
  ImGui_ImplOpenGL3_Init();
  ImGui::StyleColorsDark();
  ImGui::CreateContext();

  return window;
}

void beginFrame(GLFWwindow *window, const ivec2 &resolution) {
  ImGui_ImplOpenGL3_NewFrame();
  ImGui_ImplGlfw_NewFrame();
  ImGui::NewFrame();

  glfwGetWindowSize(window, (int *)&resolution.x, (int *)&resolution.y);

  glBindFramebuffer(GL_FRAMEBUFFER, 0);
  glViewport(0, 0, resolution.x, resolution.y);
  glEnable(GL_DEPTH_TEST);
  glDepthFunc(GL_LESS);
  glDepthMask(GL_TRUE);

  glCullFace(GL_BACK);
  glEnable(GL_CULL_FACE);

  glClearDepth(1.0);
  glClearColor(0.0, 0.0, 0.0, 1.0);
  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
}

void endFrame(GLFWwindow *window) {
  ImGui::Render();
  ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

  glfwSwapBuffers(window);

  uv_run(uv_default_loop(), UV_RUN_NOWAIT);
  glfwPollEvents();
}

int main() {
  // Setup the window/glcontext
  GLFWwindow *window = nullptr;
  ivec2 resolution(1024, 768);
  window = createWindow(resolution);
  if (window == nullptr) {
    return 1;
  }

  // Setup the RenderPipe
  renderpipe::RenderPipe &pipe = renderpipe::RenderPipe::instance();
  pipe.setFilename("../scenes/triangle.js");


  while (!glfwWindowShouldClose(window)) {
    beginFrame(window, resolution);

//    pipe.runPipeline("render");

    endFrame(window);
  }

  uv_stop(uv_default_loop());
  uv_loop_close(uv_default_loop());
  glfwTerminate();
  return 0;
}