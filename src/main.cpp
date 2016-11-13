#include "gl-wrap.h"
#include <stdio.h>

#include "vec.h"
#include "orbit-camera.h"
#include "mesher.h"

#include <shaders/built.h>

/*
  https://github.com/nothings/stb/blob/master/stb_voxel_render.h
*/

#include <iostream>
#include <math.h>
#include <string.h>


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

float window_aspect(GLFWwindow *window, int *width, int *height) {
  glfwGetFramebufferSize(window, width, height);

  float fw = static_cast<float>(*width);
  float fh = static_cast<float>(*height);

  return fabs(fw / fh);
}

void window_resize(GLFWwindow* window, int a = 0, int b = 0) {
  int width, height;

  // When reshape is called, update the view and projection matricies since this means the view orientation or size changed
  mat4_perspective(
    perspectiveMatrix,
    65.0f * (M_PI / 180.0f),
    window_aspect(window, &width, &height),
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

  Mesh *voxel_mesh = new Mesh();
  std::vector<float> out_verts;
  std::vector<unsigned int> out_faces;

  vx_mesher(volume, dims, voxel_mesh->verts, voxel_mesh->faces);

  std::cout << "verts: "
            << voxel_mesh->verts.size()
            << " elements: "
            << voxel_mesh->faces.size()
            << std::endl;

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

  voxel_mesh->upload();

  Shaders::init();

  Program *prog = new Program();
  prog->add(Shaders::get("basic.vert"))
      ->add(Shaders::get("color.frag"))
      ->output("outColor")
      ->link()
      ->use();

  prog->attribute("position");

  GLint mvpUniform = glGetUniformLocation(prog->handle, "MVP");
  GLint dimsUniform = glGetUniformLocation(prog->handle, "dims");

  // Setup the orbit camera
  vec3 eye = vec3_create(0.0f, 0.0f, camera_z);
  vec3 center = vec3_create(dims[0]/2.0f, dims[1]/2.0f, dims[2]/2.0f);
  vec3 up = vec3_create(0.0, 1.0, 0.0 );

  orbit_camera_init(eye, center, up);

  window_resize(window);

  prog->uniformVec3i("dims", dims);

  while (!glfwWindowShouldClose(window)) {
    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LESS);

    glClearColor(0.0, 0.0, 0.0, 1.0);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    orbit_camera_rotate(0, 0, -.01, .01);

    orbit_camera_view((float *)&viewMatrix);
    mat4_mul(MVP, perspectiveMatrix, viewMatrix);
    prog->uniformMat4("MVP", MVP);

    voxel_mesh->render(prog);

    glfwSwapBuffers(window);
    glfwPollEvents();
  }

  delete prog;
  delete voxel_mesh;

  Shaders::destroy();

  glfwTerminate();
  return 0;
}
