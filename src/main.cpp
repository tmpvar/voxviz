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

#include "parser/vzd/vzd.h"

#define _USE_MATH_DEFINES
#include <math.h>
#include <string.h>
#include <queue>

bool keys[1024];
bool prevKeys[1024];
double mouse[2];
bool fullscreen = 0;
// int windowDimensions[2] = { 1024, 768 };
int windowDimensions[2] = { 1440, 900 };

glm::mat4 viewMatrix, perspectiveMatrix, MVP;
Shadowmap *shadowmap;
FBO *fbo = nullptr;
FBO *shadowFBO = nullptr;

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
    (float)(width / height),
    0.001f,
    10000.0f
  );
  
  glViewport(0, 0, width, height);
  if (fbo != nullptr) {
    fbo->setDimensions(width, height);
  }

  if (shadowFBO != nullptr) {
    shadowFBO->setDimensions(width, height);
  }
  
}

/*void update_volumes(Raytracer *raytracer, Compute *compute, int time) {
  const cl_uint total = (cl_uint)raytracer->bricks.size();
  cl_mem *mem = (cl_mem *)malloc(sizeof(cl_mem) * total);
  cl_command_queue queue = compute->job.command_queues[0];

  for (cl_uint i = 0; i < total; i++) {
    mem[i] = raytracer->bricks[i]->computeBuffer;
  }

  cl_event opengl_get_completion;
  CL_CHECK_ERROR(clEnqueueAcquireGLObjects(queue, total, mem, 0, nullptr, &opengl_get_completion));
  clWaitForEvents(1, &opengl_get_completion);
  clReleaseEvent(opengl_get_completion);

  for (cl_uint i = 0; i < total; i++) {
    compute->fill("sphere", queue, raytracer->bricks[i], time);
  }

  CL_CHECK_ERROR(clEnqueueReleaseGLObjects(queue, total, mem, 0, 0, NULL));
  clFinish(queue);
  free(mem);
}
*/

/*void apply_tool(Raytracer *raytracer, Compute *compute) {
  const cl_uint total = (cl_uint)raytracer->bricks.size();
  cl_mem *mem = (cl_mem *)malloc(sizeof(cl_mem) * total);
  cl_command_queue queue = compute->job.command_queues[0];

  for (cl_uint i = 0; i < total; i++) {
    mem[i] = raytracer->bricks[i]->computeBuffer;
  }

  cl_event opengl_get_completion;
  CL_CHECK_ERROR(clEnqueueAcquireGLObjects(queue, total, mem, 0, nullptr, &opengl_get_completion));
  clWaitForEvents(1, &opengl_get_completion);
  clReleaseEvent(opengl_get_completion);
 
  compute->opCut(raytracer->bricks[0], raytracer->bricks[1]);

  CL_CHECK_ERROR(clEnqueueReleaseGLObjects(queue, total, mem, 0, 0, NULL));
  free(mem);
}
*/

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
  VolumeManager *volumeManager = new VolumeManager();
  float dt = 1.0f / 60.0f;
  q3Scene *physicsScene = new q3Scene(dt);
  // 32, 5, 16
  q3BodyDef bodyDef;
  q3BoxDef boxDef;

  memset(keys, 0, sizeof(keys));

  int d = BRICK_DIAMETER;
  float fd = (float)d;
  int dims[3] = { d, d, d };
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

  glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
  glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
  glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
  glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

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
  glfwSwapInterval(0);

  //Compute *compute = new Compute();
  if (false && glDebugMessageCallback) {
    cout << "Register OpenGL debug callback " << endl;
    glEnable(GL_DEBUG_OUTPUT_SYNCHRONOUS);
    glDebugMessageCallback(openglCallbackFunction, nullptr);
    GLuint unusedIds = 0;
    glDebugMessageControl(GL_DONT_CARE,
      GL_DONT_CARE,
      GL_DONT_CARE,
      0,
      &unusedIds,
      true);
  }
  else {
    cout << "glDebugMessageCallback not available" << endl;
  }
  Shaders::init();
  shadowmap = new Shadowmap();
  shadowmap->orient(glm::vec3(-200, 300, 260), glm::vec3(350, 60, 260));


  glfwSetWindowSize(window, windowDimensions[0], windowDimensions[1]);
  window_resize(window);

  Program *fillSphereProgram = new Program();
  fillSphereProgram
    ->add(Shaders::get("fill-sphere.comp"))
    ->link();
 
  Raytracer *raytracer = new Raytracer(dims);
  float max_distance = 10000.0f;

  glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
  glfwGetCursorPos(window, &mouse[0], &mouse[1]);
  FreeCamera *camera = new FreeCamera(
    glm::vec3(0, 2048, 2048)
  );

  int time = 0;

#if RENDER_STATIC == 1
  //update_volumes(raytracer, compute, time);
#endif

  //Volume *tool = raytracer->addVolumeAtIndex(5, 5, 5, 128, 512, 128);
  //tool->fillConst(1.0);
  /*compute->lock(compute->job.command_queues[0], tool->computeBuffer);
  compute->fill(
    "fillAll",
    compute->job.command_queues[0],
    tool,
    0
  );
  compute->unlock(compute->job.command_queues[0], tool->computeBuffer);
  */
  //tool->position(0.0, 512, 0.0);

  //clFinish(compute->job.command_queues[0]);

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

  { // query up the workgroups
    int work_grp_size[3], work_grp_inv;
    // maximum global work group (total work in a dispatch)
    glGetIntegeri_v(GL_MAX_COMPUTE_WORK_GROUP_COUNT, 0, &work_grp_size[0]);
    glGetIntegeri_v(GL_MAX_COMPUTE_WORK_GROUP_COUNT, 1, &work_grp_size[1]);
    glGetIntegeri_v(GL_MAX_COMPUTE_WORK_GROUP_COUNT, 2, &work_grp_size[2]);
    printf("max global (total) work group size x:%i y:%i z:%i\n", work_grp_size[0],
      work_grp_size[1], work_grp_size[2]);
    // maximum local work group (one shader's slice)
    glGetIntegeri_v(GL_MAX_COMPUTE_WORK_GROUP_SIZE, 0, &work_grp_size[0]);
    glGetIntegeri_v(GL_MAX_COMPUTE_WORK_GROUP_SIZE, 1, &work_grp_size[1]);
    glGetIntegeri_v(GL_MAX_COMPUTE_WORK_GROUP_SIZE, 2, &work_grp_size[2]);
    printf("max local (in one shader) work group sizes x:%i y:%i z:%i\n",
      work_grp_size[0], work_grp_size[1], work_grp_size[2]);
    // maximum compute shader invocations (x * y * z)
    glGetIntegerv(GL_MAX_COMPUTE_WORK_GROUP_INVOCATIONS, &work_grp_inv);
    printf("max computer shader invocations %i\n", work_grp_inv);
  }
  
  // TODO: if we want to enable multiple space overlapping volumes,
  //       we will need to group them together as far as physics is
  //       concerned.
  //       big volumes also cause this to halt the program.
  // bodyDef.bodyType = eDynamicBody;
  VZDParser::parse(
    "D:\\work\\voxviz\\include\\parser\\vzd\\out.vzd",
    volumeManager,
    physicsScene,
    bodyDef
  );


  q3Transform tx;
  q3Identity(tx);
  boxDef.SetRestitution(0.5);
  bodyDef.bodyType = eStaticBody;
  Volume *floor = new Volume(glm::vec3(0.0), physicsScene, bodyDef);
  floor->material = glm::vec4(0.67, 0.71, 0.78, 1.0);
  boxDef.Set(tx, q3Vec3(
    BRICK_DIAMETER,
    BRICK_DIAMETER,
    BRICK_DIAMETER
  ));

  volumeManager->addVolume(floor);

  for (int x = -16; x < 16; x++) {
    for (int y = 0; y < 1; y++) {
      for (int z = -16; z < 16; z++) {
        floor->AddBrick(glm::ivec3(x, y, z), boxDef)->createGPUMemory();
      }
    }
  }

  for (auto& it : floor->bricks) {
    Brick *brick = it.second;
    brick->fillConst(1.0);
  }

  bodyDef.bodyType = eDynamicBody;

  Volume *volume = new Volume(glm::vec3(0.0, 800.0, 0.0), physicsScene, bodyDef);
  volumeManager->addVolume(volume);
  volume->AddBrick(glm::ivec3(0, 0, 0), boxDef);
  volume->AddBrick(glm::ivec3(2, 2, 0.0), boxDef);
  volume->AddBrick(glm::ivec3(3, 1, 0), boxDef);
  volume->material = glm::vec4(1.0, 0.0, 1.0, 1.0);
   
  for (auto& it : volume->bricks) {
    Brick *brick = it.second;
    brick->createGPUMemory();
    brick->fill(fillSphereProgram);
  }
  
  glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);

  FullscreenSurface *fullscreen_surface = new FullscreenSurface();
  fbo = new FBO(windowDimensions[0], windowDimensions[1]);
  shadowFBO = new FBO(windowDimensions[0], windowDimensions[1]);
  
  uint32_t total_affected = 0;

  // Setup Dear ImGui binding
  IMGUI_CHECKVERSION();
  ImGui::CreateContext();
  //ImGuiIO& io = ImGui::GetIO(); (void)io;
  //io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;  // Enable Keyboard Controls
  //io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;   // Enable Gamepad Controls

  ImGui_ImplGlfw_InitForOpenGL(window, false);
  ImGui_ImplOpenGL3_Init();

  // Setup style
  ImGui::StyleColorsDark();

  // Start the ImGui frame
  ImGui::CreateContext();

  while (!glfwWindowShouldClose(window)) {
    if (keys[GLFW_KEY_W]) {
      camera->ProcessKeyboard(Camera_Movement::FORWARD, 1.0);
      //camera->translate(0.0f, 0.0f, 10.0f);
    }

    if (keys[GLFW_KEY_S]) {
      camera->ProcessKeyboard(Camera_Movement::BACKWARD, 1.0);
    }

    if (keys[GLFW_KEY_A]) {
      camera->ProcessKeyboard(Camera_Movement::LEFT, 1.0);
    }

    if (keys[GLFW_KEY_D]) {
      camera->ProcessKeyboard(Camera_Movement::RIGHT, 1.0);
    }

    if (keys[GLFW_KEY_H]) {
      raytracer->showHeat = 1;
    }
    else {
      raytracer->showHeat = 0;
    }

    float debug = keys[GLFW_KEY_1] ? 1.0 : 0.0;

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
        glViewport(0, 0, windowDimensions[0], windowDimensions[1]);
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

    camera->ProcessMouseMovement(delta[0], -delta[1]);
    /*
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
    */

    viewMatrix = camera->GetViewMatrix();

    MVP = perspectiveMatrix * viewMatrix;

    glm::mat4 invertedView = glm::inverse(viewMatrix);
    glm::vec3 currentEye(invertedView[3][0], invertedView[3][1], invertedView[3][2]);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glViewport(0, 0, windowDimensions[0], windowDimensions[1]);

    /*
    shadowFBO->bind();
    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LESS);
    glCullFace(GL_BACK);
    glEnable(GL_CULL_FACE);
    glClearDepth(1.0);
    glClearColor(0.0, 0.0, 0.0, 1.0);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    gl_error();

    shadowmap->begin();
    shadowmap->program
      ->use()
      ->uniformMat4("MVP", shadowmap->depthMVP)
      ->uniformVec3("eye", shadowmap->eye)
      ->uniform1i("showHeat", raytracer->showHeat)
      ->uniformFloat("maxDistance", max_distance);

      raytracer->render(shadowmap->program);
    shadowmap->end();
    shadowFBO->unbind();
    */
    
    //fbo->bind();
    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LESS);
    glDepthMask(GL_TRUE);

    glCullFace(GL_BACK);
    glEnable(GL_CULL_FACE);

    glClearDepth(1.0);
    glClearColor(0.0, 0.0, 0.0, 1.0);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    

    glm::mat4 VP = perspectiveMatrix * viewMatrix;

    for (auto& volume : volumeManager->volumes) {
      if (volume->bricks.size() == 0) {
        continue;
      }

      glm::mat4 volumeModel = volume->getModelMatrix();
      glm::vec4 invEye = glm::inverse(volumeModel) * glm::vec4(currentEye, 1.0);
      raytracer->program->use()
        ->uniformMat4("MVP", VP * volumeModel)
        ->uniformVec3("invEye", glm::vec3(
          invEye.x / invEye.w,
          invEye.y / invEye.w,
          invEye.z / invEye.w
        ))
        ->uniformFloat("maxDistance", max_distance)
        ->uniform1i("showHeat", raytracer->showHeat)
        ->uniformFloat("debug", debug)
        ->uniformVec4("material", volume->material);
      gl_error();
      volume->bind();
      raytracer->render(volume, raytracer->program);
    }
/*
    {
      glm::mat4 volumeModel = floor->getModelMatrix();
      glm::vec4 invEye = glm::inverse(volumeModel) * glm::vec4(currentEye, 1.0);
      raytracer->program->use()
        ->uniformMat4("MVP", VP * volumeModel)
        ->uniformVec3("invEye", glm::vec3(
          invEye.x / invEye.w,
          invEye.y / invEye.w,
          invEye.z / invEye.w
        ))
        ->uniformFloat("maxDistance", max_distance)
        ->uniform1i("showHeat", raytracer->showHeat)
        ->uniformFloat("debug", debug)
        ->uniformVec4("material", floor->material);

      floor->bind();
      raytracer->render(floor, raytracer->program);
    }
*/
    
    physicsScene->Step();

    
    /*fbo->unbind();
    
    glBindTexture(GL_TEXTURE_2D, shadowFBO->texture_depth);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_COMPARE_MODE, GL_COMPARE_REF_DEPTH_TO_TEXTURE_EXT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);

    fullscreen_program->use()
      ->uniformFloat("maxDistance", max_distance)
      ->texture2d("iColor", fbo->texture_color)
      ->texture2d("iPosition", fbo->texture_position)
      ->uniformVec3("light", shadowmap->eye)
      ->uniformVec3("eye", currentEye)
      ->texture2d("iShadowMap", shadowFBO->texture_depth)
      ->uniformMat4("depthBiasMVP", shadowmap->depthBiasMVP);
    
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glEnable(GL_DEPTH_TEST);

    fullscreen_surface->render(fullscreen_program);
   */
    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();
    
    {
      static float f = 0.0f;
      static int counter = 0;
      ImGui::Text("%.3f ms/frame (%.1f FPS)", 1000.0f / ImGui::GetIO().Framerate, ImGui::GetIO().Framerate);
      ImGui::Text("camera pos(%.3f, %.3f, %.3f)", camera->Position.x, camera->Position.y, camera->Position.z);
    }
    ImGui::Render();
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
    glfwSwapBuffers(window);

#if RENDER_DYNAMIC == 1
    //update_volumes(raytracer, compute, time);
#endif
    /*
    size_t total = raytracer->bricks.size();
    total_affected = 0;
    for (unsigned int i = 0; i < total; i++) {
      if (raytracer->bricks[i] == tool) {
        continue;
      }

      total_affected += compute->opCut(raytracer->bricks[i], tool);
    }
    clFinish(compute->job.command_queues[0]);
    */
    time++;
    
    shadowmap->eye.x = -1024.0f + sinf(time / 500.0f) * 1000.0f;

    if (glfwJoystickPresent(GLFW_JOYSTICK_1)) {
      glfwSetJoystickVibration(GLFW_JOYSTICK_1, total_affected, 0);
    }
    
    if (total_affected <= 1000) {
      total_affected = 0;
    }
    else {
      total_affected -= 1000;
    }

    volume->rotation.x += 0.001f;
   
    uv_run(loop, UV_RUN_NOWAIT);
    glfwPollEvents();
  }

  cout << "SHUTING DOWN" << endl;

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
