#include <stdio.h>
#include <glm/glm.hpp>

#include "camera-orbit.h"
#include "camera-free.h"
#include "raytrace.h"
#include "compute.h"
#include "core.h"
#include "clu.h"
#include "fullscreen-surface.h"
#include "shadowmap.h"

#include <shaders/built.h>
#include <iostream>

#include <uv.h>

#define _USE_MATH_DEFINES
#include <math.h>
#include <string.h>
#include <queue>

bool keys[1024];
bool prevKeys[1024];
double mouse[2];
bool fullscreen = 0;
// int windowDimensions[2] = { 1024, 768 };
int windowDimensions[2] = { 640, 480 };

glm::mat4 viewMatrix, perspectiveMatrix, MVP;
Shadowmap *shadowmap;

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
  
  shadowmap->depthProjectionMatrix = glm::perspective(
    45.0f,
    window_aspect(window, &width, &height),
    0.001f,
    10000.0f
  );
   /*
  shadowmap->depthProjectionMatrix = glm::ortho(
    -5000.0f,
    5000.0f,
    5000.0f,  
    -5000.0f,
    0.001f,
    10000.0f
  );*/


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

/* LIBUV JUNK*/
typedef struct {
  uv_buf_t buffer;
  uv_write_t request;
} write_req_t;

uv_loop_t *loop;
uv_pipe_t stdin_pipe;

static void alloc_buffer(uv_handle_t* handle, size_t suggested_size, uv_buf_t* buf) {
  buf->base = (char *)malloc(suggested_size);
  memset(buf->base, 0, suggested_size);
  buf->len = suggested_size;
}


#define MAX_LINE_LENGTH 512
char current_line[MAX_LINE_LENGTH];
unsigned int current_line_loc = 0;
const char *line_splitter = " ";
const char *line_prefix = "MACHINE_COORD ";
queue<glm::vec3> tool_position_queue;

void read_stdin(uv_stream_t* stream, ssize_t nread, const uv_buf_t* buffer) {
  if (nread > 0) {
    cout << "read " << nread << " bytes" << endl;
  }

  if (buffer->base) {
    char *base = buffer->base;
    for (ssize_t i = 0; i < nread; i++) {

      if (current_line_loc >= MAX_LINE_LENGTH) {
        current_line_loc = MAX_LINE_LENGTH - 1;
      }
      if (base[i] == '\n') {
        current_line[current_line_loc] = 0;
        char *parse_pos = strstr(current_line, line_prefix);

        if (parse_pos != NULL) {
          char *pch = strtok(parse_pos + strlen(line_prefix), line_splitter);
          glm::vec3 tool_pos;
          tool_pos[0] = 0.0f;
          float axis[3];
          int axis_loc = 0;
          while (pch != NULL) {
            tool_pos[axis_loc] = atof(pch);
            pch = strtok(NULL, line_splitter);
            axis_loc++;
            if (axis_loc > 3) {
            tool_pos[axis_loc] = atof(pch);
              break;
            }
          }

          cout << "moving tool to (" << tool_pos.x << ", " << tool_pos.y << ", " << tool_pos.z << ")" << endl;

          tool_position_queue.push(tool_pos);
        }

        memset(current_line, 0, MAX_LINE_LENGTH);
        current_line_loc = 0;
        continue;
      }
      else {
        current_line[current_line_loc] = base[i];
        current_line_loc++;
      }

    }
    free(buffer->base);
  }
}
/* LIBUV JUNK*/

int main(void) {
  memset(keys, 0, sizeof(keys));

  int d = VOLUME_DIMS;
  float fd = (float)d;
  int hd = d / 2;
  int dims[3] = { d, d, d };
  size_t total_voxels = dims[0] * dims[1] * dims[2];
  float dsquare = (float)d*(float)d;
  float camera_z = sqrtf(dsquare * 3.0f) * 1.5f;

  // libuv junk
  uv_loop_t* loop = uv_default_loop();
  uv_pipe_init(loop, &stdin_pipe, 0);
  uv_pipe_open(&stdin_pipe, 0);
  uv_read_start((uv_stream_t*)&stdin_pipe, alloc_buffer, read_stdin);
  // libuv junk

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
  shadowmap = new Shadowmap();
  glfwSetWindowSize(window, windowDimensions[0], windowDimensions[1]);
  window_resize(window);

  Raytracer *raytracer = new Raytracer(dims, compute->job);
  float max_distance = 10000.0f;
  for (float x = 0; x < 8; x++) {
    for (float y = 0; y < 1; y++) {
      for (float z = 0; z < 4; z++) {
        raytracer->addVolumeAtIndex(x, y, z, VOLUME_DIMS, VOLUME_DIMS, VOLUME_DIMS);
      }
    }
  }

  glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
  glfwGetCursorPos(window, &mouse[0], &mouse[1]);
  FreeCamera *camera = new FreeCamera();
  camera->translate(-1024, -2048, -2048);
  camera->rotate(1.0f, 0.75f, 0.0f, 0.0f);
  int time = 0;

#if RENDER_STATIC == 1
  update_volumes(raytracer, compute, time);
#endif

  Volume *tool = raytracer->addVolumeAtIndex(5, 5, 5, 128, 512, 128);
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

  Program *fullscreen_program = new Program();
  fullscreen_program
    ->add(Shaders::get("basic.vert"))
    ->add(Shaders::get("composite.frag"))
    ->output("outColor")
    ->link();

  Program *fullscreen_debug_program = new Program();
  fullscreen_debug_program
    ->add(Shaders::get("basic.vert"))
    ->add(Shaders::get("depth-to-grey.frag"))
    ->output("outColor")
    ->link();

  

  FullscreenSurface *fullscreen_surface = new FullscreenSurface();
  FBO *fbo = new FBO(windowDimensions[0], windowDimensions[1]);
  FBO *shadowFBO = new FBO(windowDimensions[0], windowDimensions[1]);
  
  uint32_t total_affected = 0;

  while (!glfwWindowShouldClose(window)) {
    if (keys[GLFW_KEY_W]) {
      camera->translate(0.0f, 0.0f, 10.0f);
    }

    if (keys[GLFW_KEY_S]) {
      camera->translate(0.0f, 0.0f, -10.0f);
    }

    if (keys[GLFW_KEY_A]) {
      camera->translate(5.0f, 0.0f, 0.0f);
    }

    if (keys[GLFW_KEY_D]) {
      camera->translate(-5.0f, 0.0f, 0.0f);
    }

    if (keys[GLFW_KEY_H]) {
      raytracer->showHeat = 1;
    }
    else {
      raytracer->showHeat = 0;
    }

    if (keys[GLFW_KEY_LEFT]) {
      camera->yaw(-0.02f);
    }

    if (keys[GLFW_KEY_RIGHT]) {
      camera->yaw(0.02f);
    }

    if (keys[GLFW_KEY_UP]) {
      camera->pitch(-0.02f);
    }

    if (keys[GLFW_KEY_DOWN]) {
      camera->pitch(0.02f);
    }

    if (!keys[GLFW_KEY_ENTER] && prevKeys[GLFW_KEY_ENTER]) {
      if (!fullscreen) {
        const GLFWvidmode *mode = glfwGetVideoMode(glfwGetPrimaryMonitor());
        glfwSetWindowSize(window, mode->width, mode->height);
        glfwSetWindowPos(window, 0, 0);
        glfwSetWindowMonitor(window, glfwGetPrimaryMonitor(), 0, 0, mode->width, mode->height, GLFW_DONT_CARE);
        glViewport(0, 0, mode->width, mode->height);
        fullscreen = true;
      }
      else {
        glfwSetWindowSize(window, windowDimensions[0], windowDimensions[1]);
        glfwSetWindowPos(window, 100, 100);
        glfwSetWindowMonitor(window, NULL, 100, 100, windowDimensions[0], windowDimensions[1], GLFW_DONT_CARE);
        glViewport(0, 0, windowDimensions[0], windowDimensions[0]);
        fullscreen = false;
      }
      prevKeys[GLFW_KEY_ENTER] = false;

      delete fbo;
      delete shadowFBO;
      glfwGetWindowSize(window, &windowDimensions[0], &windowDimensions[1]);
      fbo = new FBO(windowDimensions[0], windowDimensions[1]);
      shadowFBO = new FBO(windowDimensions[0], windowDimensions[1]);

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

      tool->move(
        axis[2] * 5.0f,
        axis[1] * 5.0f,
        axis[3] * -5.0f
      );
    }

    else if (false) {
      tool->position(
        256.0f * 2.0f + sinf((float)time / 50.0f) * fmod(time, 3000.0f) / 5.0f,
        512.0f - time / 100.0f,
        256.0f * 2.0f + cosf((float)time / 50.0f) * fmod(time, 3000.0f) / 5.0f
      );
    }

    if (tool_position_queue.size() > 0) {
      glm::vec3 new_tool_pos = glm::abs(tool_position_queue.front());
      tool_position_queue.pop();
      tool->position(
        new_tool_pos.x * 10.0f,
        new_tool_pos.z * 10.0f,
        new_tool_pos.y * 10.0f
      );
    }

    viewMatrix = camera->view();

    MVP = perspectiveMatrix * viewMatrix;

    glm::mat4 invertedView = glm::inverse(viewMatrix);
    glm::vec3 currentEye(invertedView[3][0], invertedView[3][1], invertedView[3][2]);

    shadowFBO->bind();
    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LESS);
    glDepthMask(GL_TRUE);

    glCullFace(GL_BACK);
    glEnable(GL_CULL_FACE);

    glClearDepth(1.0);
    glClearColor(0.0, 0.0, 0.0, 1.0);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    gl_error();
    shadowmap->bindCollect(shadowmap->program, shadowFBO);
    raytracer->render(
      shadowmap->program->use(),
      shadowmap->depthMVP,
      shadowmap->eye,
      max_distance
    );
    shadowFBO->unbind();
    
    fbo->bind();
    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LESS);
    glDepthMask(GL_TRUE);

    glCullFace(GL_BACK);
    glEnable(GL_CULL_FACE);

    glClearDepth(1.0);
    glClearColor(0.0, 0.0, 0.0, 1.0);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    raytracer->program->use()
      ->uniformMat4("depthBiasMVP", shadowmap->depthBiasMVP)
      ->uniformVec3("light", shadowmap->eye)
      ->texture2d("shadowMap", shadowFBO->texture_depth);
    

    raytracer->render(
      raytracer->program,
      MVP,
      currentEye,
      max_distance
    );
    fbo->unbind();
    
    fullscreen_program->use()->uniformFloat("maxDistance", max_distance);
    if (keys[GLFW_KEY_1]) {
      fullscreen_program->texture2d("color", shadowFBO->texture_depth);
    } else {
      fullscreen_program->texture2d("color", fbo->texture_color);
    }
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glEnable(GL_DEPTH_TEST);
    fullscreen_surface->render(fullscreen_program);
    
    glfwSwapBuffers(window);

#if RENDER_DYNAMIC == 1
    update_volumes(raytracer, compute, time);
#endif
    glFlush();

    size_t total = raytracer->volumes.size();
    total_affected = 0;
    for (unsigned int i = 0; i < total; i++) {
      if (raytracer->volumes[i] == tool) {
        continue;
      }

      total_affected += compute->opCut(raytracer->volumes[i], tool);
    }
    clFinish(compute->job.command_queues[0]);
 
    time++;
    
    if (glfwJoystickPresent(GLFW_JOYSTICK_1)) {
      glfwSetJoystickVibration(GLFW_JOYSTICK_1, total_affected, 0);
    }
    
    if (total_affected <= 1000) {
      total_affected = 0;
    }
    else {
      total_affected -= 1000;
    }


    uv_run(loop, UV_RUN_NOWAIT);
    glfwPollEvents();
  }

  delete fullscreen_program;
  delete fullscreen_surface;
  delete raytracer;
  delete shadowmap;
  Shaders::destroy();
  uv_read_stop((uv_stream_t*)&stdin_pipe);
  uv_stop(uv_default_loop());
  uv_loop_close(uv_default_loop());
  glfwTerminate();
  return 0;
}
