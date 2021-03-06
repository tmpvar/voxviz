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
  uv_pipe_init(uv_default_loop(), &stdin_pipe, 0);
  uv_pipe_open(&stdin_pipe, 0);
  uv_read_start((uv_stream_t*)&stdin_pipe, alloc_buffer, read_stdin);
  // libuv junk

  GLFWwindow* window;

  if (!glfwInit()) {
    return -1;
  }

  glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
  glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 6);
  //glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
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
  shadowmap->orient(glm::vec3(-50, 50, 50), glm::vec3(25, 10, 30));


  glfwSetWindowSize(window, windowDimensions[0], windowDimensions[1]);
  window_resize(window);

  Program *fillSphereProgram = new Program();
  fillSphereProgram
    ->add(Shaders::get("fill-sphere.comp"))
    ->link();


  Program *fillAllProgram = new Program();
  fillAllProgram
    ->add(Shaders::get("fill-all.comp"))
    ->link();


  Program *brickCutProgram = new Program();
  brickCutProgram
    ->add(Shaders::get("brick-cut.comp"))
    ->link();

  Program *brickAddProgram = new Program();
  brickAddProgram
    ->add(Shaders::get("brick-add.comp"))
    ->link();


  Raytracer *raytracer = new Raytracer(dims);
  float max_distance = 10000.0f;

  glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
  glfwGetCursorPos(window, &mouse[0], &mouse[1]);
  FreeCamera *camera = new FreeCamera(
    glm::vec3(10, 40, 30)
  );

  int time = 0;

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

  /*VZDParser::parse(
    "D:\\work\\voxviz\\include\\parser\\vzd\\out.vzd",
    volumeManager,
    physicsScene,
    bodyDef
  );

  VOXParser::parse(
    "D:\\work\\voxel-model\\vox\\character\\chr_cat.vox",
    volumeManager,
    physicsScene,
    bodyDef
  );*/

  Volume *tool = new Volume(glm::vec3(-5.0, 0 , 0.0));

  Brick *toolBrick = tool->AddBrick(glm::ivec3(1, 0, 0), &boxDef);
  toolBrick->createGPUMemory();
  toolBrick->fill(fillSphereProgram);



  Brick *toolBrick2 = tool->AddBrick(glm::ivec3(2, 0, 0), &boxDef);
  toolBrick2->createGPUMemory();
  toolBrick2->fillConst(0xFFFFFFFF);


  volumeManager->addVolume(tool);


  q3Transform tx;
  q3Identity(tx);
  boxDef.SetRestitution(0.5);
  bodyDef.bodyType = eStaticBody;

  boxDef.Set(tx, q3Vec3(
    BRICK_DIAMETER,
    BRICK_DIAMETER,
    BRICK_DIAMETER
  ));

  Volume *floor = new Volume(glm::vec3(0.0));
  floor->material = glm::vec4(0.67, 0.71, 0.78, 1.0);
  //Brick *floorBrick = floor->AddBrick(glm::ivec3(1, 0, 0));
  //floorBrick->createGPUMemory();
  //floorBrick->fillConst(1.0);
  //floor->cut(tool);
  //return 1;

  tool->scale.x = 4.0;
  tool->scale.y = 4.0;
  tool->scale.z = 4.0;
 // tool->rotation.z = M_PI / 4.0;
  //floor->rotation.z = M_PI / 2.0;

  volumeManager->addVolume(floor);

  for (int x = 0; x < 128; x++) {
    for (int y = 0; y < 64; y++) {
      for (int z = 0; z < 128; z++) {
        floor->AddBrick(glm::ivec3(x, y, z));
      }
    }
  }

  //fillAllProgram->use()->uniform1ui("val", 0xFFFFFFFF);
  size_t i = 0;
  for (auto& it : floor->bricks) {
    i++;
    Brick *brick = it.second;
    brick->createGPUMemory();
    i % 5 > 0 ? brick->fillConst(0xFFFFFFFF) : brick->fill(fillSphereProgram);
    //brick->fill(fillSphereProgram);
   // brick->fill(fillAllProgram);
  }


  bodyDef.bodyType = eDynamicBody;

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
  const float movementSpeed = 0.1f;
  GLFWgamepadstate state;
  while (!glfwWindowShouldClose(window)) {

    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();

    // input
    {
      float speed = movementSpeed * (keys[GLFW_KEY_LEFT_SHIFT] ? 10.0 : 1.0);
      if (keys[GLFW_KEY_W]) {
        camera->ProcessKeyboard(Camera_Movement::FORWARD, speed);
        //camera->translate(0.0f, 0.0f, 10.0f);
      }

      if (keys[GLFW_KEY_S]) {
        camera->ProcessKeyboard(Camera_Movement::BACKWARD, speed);
      }

      if (keys[GLFW_KEY_A]) {
        camera->ProcessKeyboard(Camera_Movement::LEFT, speed);
      }

      if (keys[GLFW_KEY_D]) {
        camera->ProcessKeyboard(Camera_Movement::RIGHT, speed);
      }

      if (keys[GLFW_KEY_H]) {
        raytracer->showHeat = 1;
      }
      else {
        raytracer->showHeat = 0;
      }

      float debug = keys[GLFW_KEY_1] ? 1.0f : 0.0f;

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

      camera->ProcessMouseMovement((float)delta[0], (float)-delta[1]);

      if (glfwGetGamepadState(GLFW_JOYSTICK_1, &state)) {
        int axis_count;
        const float *axis = glfwGetJoystickAxes(GLFW_JOYSTICK_1, &axis_count);
        const float x = state.axes[GLFW_GAMEPAD_AXIS_RIGHT_X];
        const float y = state.axes[GLFW_GAMEPAD_AXIS_LEFT_Y];
        const float z = state.axes[GLFW_GAMEPAD_AXIS_RIGHT_Y];
        tool->move(
          fabs(x) > 0.1 ? x * movementSpeed : 0,
          fabs(y) > 0.1 ? -y * movementSpeed : 0,
          fabs(z) > 0.1 ? z * movementSpeed : 0
        );
      }
    }

    if (tool_position_queue.size() > 0) {
      glm::vec3 new_tool_pos = glm::abs(tool_position_queue.front());
      tool_position_queue.pop();
      tool->position = glm::vec3(
        new_tool_pos.x * 10.0f,
        new_tool_pos.z * 10.0f,
        new_tool_pos.y * 10.0f
      );
    }


    viewMatrix = camera->GetViewMatrix();

    MVP = perspectiveMatrix * viewMatrix;

    glm::mat4 invertedView = glm::inverse(viewMatrix);
    glm::vec3 currentEye(invertedView[3][0], invertedView[3][1], invertedView[3][2]);
    glfwGetWindowSize(window, &windowDimensions[0], &windowDimensions[1]);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glViewport(0, 0, windowDimensions[0], windowDimensions[1]);

    if (!shaderLogs.empty()) {
      ImGui::SetNextWindowPos(ImVec2(20, windowDimensions[1] / 2 + 20));
    }
    else {
      ImGui::SetNextWindowPos(ImVec2(20, 20));
    }

    ImGui::SetNextWindowSize(ImVec2(300, windowDimensions[1] / 2 - 20));
    ImGui::Begin("stats");

    // render shadow map
    if (false) {
      Timer shadowMapTimer;
      shadowMapTimer.begin("shadowmap");
      shadowFBO->bind();
      glDrawBuffer(GL_NONE);
      glEnable(GL_DEPTH_TEST);
      glDepthFunc(GL_LESS);
      glCullFace(GL_BACK);
      glEnable(GL_CULL_FACE);
      glClearDepth(1.0);
      glClearColor(0.0, 0.0, 0.0, 1.0);
      glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
      gl_error();

      shadowmap->orient(
        tool->position,
        tool->position + glm::vec3(50, 0, 0)
      );

      shadowmap->begin();
      for (auto& volume : volumeManager->volumes) {
        if (volume->bricks.size() == 0) {
          continue;
        }

        glm::mat4 volumeModel = volume->getModelMatrix();
        glm::vec4 invEye = glm::inverse(volumeModel) * glm::vec4(shadowmap->eye, 1.0);

        shadowmap->program
          ->use()
          ->uniformMat4("MVP", shadowmap->depthMVP)
          ->uniformVec3("invEye", glm::vec3(
            invEye.x / invEye.w,
            invEye.y / invEye.w,
            invEye.z / invEye.w
          ))
          ->uniformMat4("model", volumeModel)
          ->uniformVec3("eye", shadowmap->eye)
          ->uniformFloat("maxDistance", max_distance)
          ->uniform1i("showHeat", raytracer->showHeat)
          ->uniformVec4("material", volume->material);

        raytracer->render(volume, shadowmap->program);
      }
      shadowmap->end();
      shadowFBO->unbind();
      shadowMapTimer.end()->waitUntilComplete()->debug();
    }

    // primary rays + proxy geometry rasterization
    if (true) {
      Timer primaryRaysTimer;
      primaryRaysTimer.begin("primary rays");
      fbo->bind();
      glEnable(GL_DEPTH_TEST);
      glDepthFunc(GL_LESS);
      glDepthMask(GL_TRUE);

      glCullFace(GL_BACK);
      glEnable(GL_CULL_FACE);

      glClearDepth(1.0);
      glClearColor(0.0, 0.0, 0.0, 1.0);
      glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);


      glm::mat4 VP = perspectiveMatrix * viewMatrix;

      for (int i = 0; i < 2048; i++) {
        if (volumeManager->tick()) {
          break;
        }
      }

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
          ->uniformMat4("model", volumeModel)
          ->uniformVec3("eye", currentEye)
          ->uniformFloat("maxDistance", max_distance)
          ->uniform1i("showHeat", raytracer->showHeat)
          ->uniformVec4("material", volume->material);

        raytracer->render(volume, raytracer->program);
      }

      physicsScene->Step();
      //floor->rotation.z += 0.001;
      tool->rotation.z += 0.001;
      tool->rotation.y += 0.002;
      //tool->rotation.x += 0.005;

      fbo->unbind();
      primaryRaysTimer.end()->waitUntilComplete()->debug();
    }

    // render with shadow buffer
    if (true) {
      Timer compositeTimer;
      compositeTimer.begin("composite");
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
        ->texture2d("iShadowColor", shadowFBO->texture_color)
        ->texture2d("iShadowPosition", shadowFBO->texture_position)
        ->uniformMat4("depthBiasMVP", shadowmap->depthBiasMVP);

      glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
      glEnable(GL_DEPTH_TEST);

      fullscreen_surface->render(fullscreen_program);
      compositeTimer.end()->waitUntilComplete()->debug();
  }

    // Tool based boolean operations
    // opCut
    if (state.axes[GLFW_GAMEPAD_AXIS_RIGHT_TRIGGER] > 0.1) {
      for (auto& volume : volumeManager->volumes) {
        if (volume == tool) {
          continue;
        }

        volume->booleanOp(tool, false, brickCutProgram);
      }
    }

    if (state.buttons[GLFW_GAMEPAD_BUTTON_RIGHT_BUMPER] || keys[GLFW_KEY_SPACE]) {
      floor->booleanOp(tool, true, brickAddProgram);
    }

    // display stats
    {
      static float f = 0.0f;
      static int counter = 0;
      ImGui::Text("%.3f ms/frame (%.1f FPS)", 1000.0f / ImGui::GetIO().Framerate, ImGui::GetIO().Framerate);
      ImGui::Text("camera pos(%.3f, %.3f, %.3f)", camera->Position.x, camera->Position.y, camera->Position.z);
      ImGui::Text(
        "tool pos(%.3f, %.3f, %.3f)",
        tool->position.x,
        tool->position.y,
        tool->position.z
      );
      ImGui::Text("%i floor bricks", floor->bricks.size());
      ImGui::End();

      if (!shaderLogs.empty()) {
        bool yes = 1;
        ImGui::SetNextWindowPos(ImVec2(0, 0));
        ImGui::SetNextWindowSize(ImVec2(windowDimensions[0], windowDimensions[1] / 2));
        ImGui::Begin(
          "Shader Errors",
          &yes,
          ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoTitleBar
        );
        for (auto const& log : shaderLogs) {
          unsigned int lines = std::count(log.second.begin(), log.second.end(), '\n');
          ImGui::BeginChild(log.first.c_str(), ImVec2(0, 20 + 20 * lines), true);
          ImGui::Text(log.first.c_str());
          ImGui::TextColored(ImVec4(1.0, 0.0, 0.0, 1.0), log.second.c_str());
          ImGui::EndChild();
        }
        ImGui::End();
      }
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

    // TODO: not in glfw 3.3
    /*if (glfwJoystickPresent(GLFW_JOYSTICK_1)) {
      glfwSetJoystickVibration(GLFW_JOYSTICK_1, total_affected, 0);
    }*/

    if (total_affected <= 1000) {
      total_affected = 0;
    }
    else {
      total_affected -= 1000;
    }

    uv_run(uv_default_loop(), UV_RUN_NOWAIT);
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
