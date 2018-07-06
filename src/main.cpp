#include <stdio.h>
#include <glm/glm.hpp>

#include "camera-orbit.h"
#include "camera-free.h"
#include "raytrace.h"
#include "compute.h"
#include "core.h"
#include "clu.h"

#include <shaders/built.h>
#include <iostream>

#define _USE_MATH_DEFINES
#include <math.h>
#include <string.h>

bool keys[1024];
bool prevKeys[1024];
double mouse[2];
bool fullscreen = 0;
// int windowDimensions[2] = { 1024, 768 };
int windowDimensions[2] = { 640, 480 };

glm::mat4 viewMatrix, perspectiveMatrix, MVP;

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
    45.0f,
    window_aspect(window, &width, &height),
    0.1f,
    10000.0f
  );

  glViewport(0, 0, width, height);
}

void update_volumes(Raytracer *raytracer, Compute *compute, int time) {
  const cl_uint total = (cl_uint)raytracer->volumes.size();
  cl_mem *mem = (cl_mem *)malloc(sizeof(cl_mem) * total);
  cl_command_queue queue = compute->job.command_queues[0];

  for (cl_uint i = 0; i < total; i++) {
    mem[i] = raytracer->volumes[i]->computeBuffer;
  }

  cl_event opengl_get_completion;
  CL_CHECK_ERROR(clEnqueueAcquireGLObjects(queue, total, mem, 0, nullptr, &opengl_get_completion));
  clWaitForEvents(1, &opengl_get_completion);
  clReleaseEvent(opengl_get_completion);

  for (cl_uint i = 0; i < total; i++) {
    compute->fill("fillAll", queue, raytracer->volumes[i], time);
  }

  CL_CHECK_ERROR(clEnqueueReleaseGLObjects(queue, total, mem, 0, 0, NULL));

  free(mem);
}


void apply_tool(Raytracer *raytracer, Compute *compute) {
  const cl_uint total = (cl_uint)raytracer->volumes.size();
  cl_mem *mem = (cl_mem *)malloc(sizeof(cl_mem) * total);
  cl_command_queue queue = compute->job.command_queues[0];

  for (cl_uint i = 0; i < total; i++) {
    mem[i] = raytracer->volumes[i]->computeBuffer;
  }

  cl_event opengl_get_completion;
  CL_CHECK_ERROR(clEnqueueAcquireGLObjects(queue, total, mem, 0, nullptr, &opengl_get_completion));
  clWaitForEvents(1, &opengl_get_completion);
  clReleaseEvent(opengl_get_completion);
 
  compute->opCut(raytracer->volumes[0], raytracer->volumes[1]);

  CL_CHECK_ERROR(clEnqueueReleaseGLObjects(queue, total, mem, 0, 0, NULL));
  free(mem);
}

int main(void) {
  memset(keys, 0, sizeof(keys));

  int d = VOLUME_DIMS;
  float fd = (float)d;
  int hd = d / 2;
  int dims[3] = { d, d, d };
  size_t total_voxels = dims[0] * dims[1] * dims[2];
  float dsquare = (float)d*(float)d;
  float camera_z = sqrtf(dsquare * 3.0f) * 1.5f;

  GLFWwindow* window;

  if (!glfwInit()) {
    return -1;
  }

  glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
  glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 2);
  glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
  glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
  glfwSwapInterval(0);

#ifndef FULLSCREEN
  window = glfwCreateWindow(windowDimensions[0], windowDimensions[1], "voxviz", NULL, NULL);
#else
  // fullscreen
  GLFWmonitor* primary = glfwGetPrimaryMonitor();
  const GLFWvidmode* mode = glfwGetVideoMode(primary);
  window = glfwCreateWindow(mode->width, mode->height, "voxviz", glfwGetPrimaryMonitor(), NULL);
  glfwSetWindowSizeCallback(window, window_resize);
#endif

  if (!window) {
    glfwTerminate();
    return -1;
  }

  glfwSetKeyCallback(window, key_callback);
  glfwMakeContextCurrent(window);
  gladLoadGLLoader((GLADloadproc)glfwGetProcAddress);

  Compute *compute = new Compute();

  Shaders::init();

  Program *prog = new Program();
  prog->add(Shaders::get("basic.vert"))
      ->add(Shaders::get("color.frag"))
      ->output("outColor")
      ->link()
      ->use();

  // Setup the orbit camera
  glm::vec3 center(256.0, 0.0, 0.0);
  glm::vec3 eye(0.0, 0.0, -glm::length(center) * 2.0);

  glm::vec3 up(0.0, 1.0, 0.0);

  window_resize(window);

  Raytracer *raytracer = new Raytracer(dims, compute->job);

  for (float x = 0; x < 4; x++) {
    //for (float y = 0; y < VOLUME_SIDE; y++) {
      for (float z = 0; z < 4; z++) {
        raytracer->addVolumeAtIndex(x, 0.0, z, VOLUME_DIMS, VOLUME_DIMS, VOLUME_DIMS);
      }
    //}
  }
  orbit_camera_init(eye, center, up);

  glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
  glfwGetCursorPos(window, &mouse[0], &mouse[1]);
  FreeCamera *camera = new FreeCamera();
  int time = 0;

#if RENDER_STATIC == 1
  update_volumes(raytracer, compute, time);
#endif

  Volume *tool = raytracer->addVolumeAtIndex(5, 5, 5, 32, 128, 64);
  compute->lock(compute->job.command_queues[0], tool->computeBuffer);
  compute->fill(
    "cylinder",
    compute->job.command_queues[0],
    tool,
    0
  );
  compute->unlock(compute->job.command_queues[0], tool->computeBuffer);
  tool->position(0.0, 128, 0.0);

  clFinish(compute->job.command_queues[0]);

  while (!glfwWindowShouldClose(window)) {
    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LESS);
    glDepthMask(GL_TRUE);
    
    //glCullFace(GL_BACK);
    //glEnable(GL_CULL_FACE);

    glClearDepth(1.0);
    glClearColor(0.0, 0.0, 0.0, 1.0);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    if (keys[GLFW_KEY_W]) {
      camera->translate(0.0f, 0.0f, 10.0f);
      orbit_camera_zoom(-5.0f);
    }

    if (keys[GLFW_KEY_S]) {
      camera->translate(0.0f, 0.0f, -10.0f);
      orbit_camera_zoom(5.0f);
    }

    if (keys[GLFW_KEY_A]) {
      camera->translate(5.0f, 0.0f, 0.0f);
      orbit_camera_zoom(-5.0f);
    }

    if (keys[GLFW_KEY_D]) {
      camera->translate(-5.0f, 0.0f, 0.0f);
      orbit_camera_zoom(5.0f);
    }

    if (keys[GLFW_KEY_H]) {
      raytracer->showHeat = 1;
    }
    else {
      raytracer->showHeat = 0;
    }

    if (keys[GLFW_KEY_LEFT]) {
      orbit_camera_rotate(0.0f, 0.0f, 0.02f, 0.0f);
      camera->yaw(-0.02f);
    }

    if (keys[GLFW_KEY_RIGHT]) {
      orbit_camera_rotate(0.0f, 0.0f, -0.02f, 0.0f);
      camera->yaw(0.02f);
    }

    if (keys[GLFW_KEY_UP]) {
      orbit_camera_rotate(0.0f, 0.0f, 0.0f, -0.02f);
      camera->pitch(-0.02f);
    }

    if (keys[GLFW_KEY_DOWN]) {
      orbit_camera_rotate(0.0f, 0.0f, 0.0f, 0.02f);
      camera->pitch(0.02f);
    }

    if (!keys[GLFW_KEY_ENTER] && prevKeys[GLFW_KEY_ENTER]) {
      if (!fullscreen) {
        const GLFWvidmode *mode = glfwGetVideoMode(glfwGetPrimaryMonitor());
        glfwSetWindowSize(window, mode->width, mode->height);
        glfwSetWindowPos(window, 0, 0);
        fullscreen = true;
      }
      else {
        glfwSetWindowSize(window, windowDimensions[0], windowDimensions[1]);
        fullscreen = false;
      }
      prevKeys[GLFW_KEY_ENTER] = false;
    }

    // mouse look like free-fly/fps
    double xpos, ypos, delta[2];
    glfwGetCursorPos(window, &xpos, &ypos);
    delta[0] = xpos - mouse[0];
    delta[1] = ypos - mouse[1];
    mouse[0] = xpos;
    mouse[1] = ypos;

    camera->pitch((float)(delta[1] / 500.0));
    camera->yaw((float)(delta[0] / 500.0));
  

    if (glfwJoystickPresent(GLFW_JOYSTICK_1)) {
      int axis_count;
      const float *axis = glfwGetJoystickAxes(GLFW_JOYSTICK_1, &axis_count);

      camera->translate(axis[0] * -5.0f, 0.0f, axis[1] * 5.0f);

      camera->pitch(-axis[3]/20.0f);
      camera->yaw(axis[2]/20.0f);
    }

    viewMatrix = camera->view();

     MVP = perspectiveMatrix * viewMatrix;

    glm::mat4 invertedView = glm::inverse(viewMatrix);
    glm::vec3 currentEye(invertedView[3][0], invertedView[3][1], invertedView[3][2]);
    raytracer->render(MVP, currentEye);

    glfwSwapBuffers(window);

#if RENDER_DYNAMIC == 1
    update_volumes(raytracer, compute, time);
#endif
    glFinish();

    tool->position(
      128.0f * 2.0f + sinf((float)time / 50.0f) * fmod(time, 3000.0f) / 10.0f,
      128.0f - time / 100.0f,
      128.0f * 2.0f + cosf((float)time / 50.0f) * fmod(time, 3000.0f) / 10.0f
    );


    size_t total = raytracer->volumes.size();
    for (unsigned int i = 0; i < total; i++) {
      if (raytracer->volumes[i] == tool) {
        continue;
      }

      compute->opCut(raytracer->volumes[i], tool);
    }
    clFinish(compute->job.command_queues[0]);
 
    time++;

    glfwPollEvents();
  }

  delete prog;
  delete raytracer;

  Shaders::destroy();

  glfwTerminate();
  return 0;
}
