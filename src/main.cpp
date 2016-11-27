#include "gl-wrap.h"
#include <stdio.h>
#include <glm/glm.hpp>

#include "orbit-camera.h"
#include "mesher.h"
#include "raytrace.h"
#include "compute.h"

#include <shaders/built.h>

/*
  https://github.com/nothings/stb/blob/master/stb_voxel_render.h
*/

#include <iostream>

#define _USE_MATH_DEFINES
#include <math.h>
#include <string.h>


#define vol(i, j, k) volume[i + dims[0] * (j + dims[1] * k)]

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
  int *volume = (int *)malloc(total_voxels * sizeof(int));
  // memset(volume, 0, total_voxels * sizeof(int));
  //
  float camera_z = sqrt(d*d + d*d + d*d) * 1.5;
  //
  // for (int x=0; x<dims[0]; x++) {
  //   for (int y=0; y<dims[1]; y++) {
  //     for (int z=0; z<dims[2]; z++) {
  //
  //       int dx = x - hd;
  //       int dy = y - hd;
  //       int dz = z - hd;
  //
  //       vol(x, y, z) = sqrt(dx*dx + dy*dy + dz*dz) - (hd + 8) > 0 ? 255 : 0;
  //     }
  //   }
  // }

  GLFWwindow* window;

  if (!glfwInit()) {
    return -1;
  }

  glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
  glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 2);
  glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
  glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

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
  // std::vector<float> out_verts;
  // std::vector<unsigned int> out_faces;

  // vx_mesher(volume, dims, voxel_mesh->verts, voxel_mesh->faces);

  // std::cout << "verts: "
  //           << voxel_mesh->verts.size()
  //           << " elements: "
  //           << voxel_mesh->faces.size()
  //           << std::endl;

  // voxel_mesh->upload();

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
  glm::vec3 eye(0.0, 0.0, camera_z);
  glm::vec3 center(dims[0]/2.0f, dims[1]/2.0f, dims[2]/2.0f);
  glm::vec3 up(0.0, 1.0, 0.0);

  orbit_camera_init(eye, center, up);

  window_resize(window);

  Raytracer *raytracer = new Raytracer(dims, compute->job);
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
      orbit_camera_rotate(-.01, 0, 0, 0);
    }

    if (keys[GLFW_KEY_RIGHT]) {
      orbit_camera_rotate(.01, 0, 0, 0);
    }

    if (keys[GLFW_KEY_UP]) {
      orbit_camera_rotate(0, 0, 0, -.01);
    }

    if (keys[GLFW_KEY_DOWN]) {
      orbit_camera_rotate(.01, 0, 0, 0.01);
    }

    time++;
    for (int i=0; i<VOLUME_COUNT; i++) {
      compute->fill(raytracer->volumes[i]->computeBuffer, time);
    }

    // orbit_camera_rotate(0, 0, -.01, .01);
    viewMatrix = orbit_camera_view();
    MVP = perspectiveMatrix * viewMatrix;

    // prog->use()->uniformMat4("MVP", MVP)->uniformVec3i("dims", dims);;
    // voxel_mesh->render(prog, "position");

    glm::mat4 invertedView = glm::inverse(viewMatrix);
    glm::vec3 currentEye(invertedView[3][0], invertedView[3][1], invertedView[3][2]);
    raytracer->render(MVP, currentEye);

    glfwSwapBuffers(window);
    glfwPollEvents();
  }

  delete prog;
  delete voxel_mesh;
  delete raytracer;

  Shaders::destroy();

  glfwTerminate();
  return 0;
}
