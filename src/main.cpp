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
#include "greedy-mesher.h"

#define _USE_MATH_DEFINES
#include <math.h>
#include <sstream>
#include <string.h>
#include <queue>
#include <map>

bool keys[1024];
bool prevKeys[1024];
double mouse[2];
bool fullscreen = 0;
bool yes = true;
const float fov = 45.0f;
// int windowDimensions[2] = { 1024, 768 };
int windowDimensions[2] = { 1440, 900 };
glm::mat4 viewMatrix, perspectiveMatrix, MVP;
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

int main(void) {
  memset(keys, 0, sizeof(keys));

  int d = BRICK_DIAMETER;
  float fd = (float)d;
  int dims[3] = { d, d, d };
  float dsquare = (float)d*(float)d;
  float camera_z = sqrtf(dsquare * 3.0f) * 1.5f;

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
#endif


  if (!window) {
    glfwTerminate();
    return -1;
  }

  glfwSetWindowSizeCallback(window, window_resize);
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
      GL_DEBUG_SEVERITY_MEDIUM,
      0,
      &unusedIds,
      true);
  }
  else {
    cout << "glDebugMessageCallback not available" << endl;
  }

  Shaders::init();

  glfwSetWindowSize(window, windowDimensions[0], windowDimensions[1]);
  window_resize(window);

  float max_distance = 10000.0f;

  glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
  glfwGetCursorPos(window, &mouse[0], &mouse[1]);
  FreeCamera *camera = new FreeCamera(
    glm::vec3(100, 100, -200),
	glm::vec3(0.0, 1.0, 0.0),
	90.0,
	0.0
  );

  unsigned int time = 0;

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

  FullscreenSurface *fullscreen_surface = new FullscreenSurface();
  fbo = new FBO(windowDimensions[0], windowDimensions[1]);


  // Setup Dear ImGui binding
  IMGUI_CHECKVERSION();
  ImGui::CreateContext();
  ImGui_ImplGlfw_InitForOpenGL(window, false);
  ImGui_ImplOpenGL3_Init();
  ImGui::StyleColorsDark();
  ImGui::CreateContext();

  const float movementSpeed = 0.01f;
  GLFWgamepadstate state;

  // Voxel space
  const glm::uvec3 voxelSpaceDims = glm::uvec3(1024, 128, 1024);

  uint64_t voxelSpaceBytes =
    static_cast<uint64_t>(voxelSpaceDims.x) *
    static_cast<uint64_t>(voxelSpaceDims.y) *
    static_cast<uint64_t>(voxelSpaceDims.z);

  uint64_t total_voxel_slab_slots = 0;
  for (unsigned int i = 0; i <= MAX_MIP_LEVELS; i++) {
    uint64_t a = voxelSpaceBytes >> (i * 3);
    total_voxel_slab_slots += a;
  }

  SSBO *voxelSpaceSSBO = new SSBO(
    total_voxel_slab_slots, // 1 byte per voxel.. for now
    voxelSpaceDims
  );


  VOXModel *cyl = VOXParser::parse("E:\\gfx\\voxviz\\img\\models\\car.vox");
  Mesh *cylMesh = new Mesh();

  if (cyl == nullptr) {
    cout << "cannot find cyl.vox" << endl;
    return 1;
  }
  std::vector<uvec3> vertices;
  std::vector<uvec4> faces;
  greedy(cyl->buffer, ivec3(cyl->dims), cylMesh);
  cylMesh->upload();

  BlueNoise1x64x64x64 *blue_noise = new BlueNoise1x64x64x64();
  blue_noise->upload();

  double deltaTime = 0.0;
  double lastTime = glfwGetTime();
  float debug = 0.0;
  glm::vec3 characterPos(20.0);
  int lastCharacterTime = -1;
  glm::vec3 lastCharacterPos(20.0);
  while (!glfwWindowShouldClose(window)) {
    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();

    glfwGetWindowSize(window, &windowDimensions[0], &windowDimensions[1]);

    if (!shaderLogs.empty()) {
      ImGui::SetNextWindowPos(ImVec2(20, windowDimensions[1] / 2 + 20));
      ImGui::SetNextWindowSize(ImVec2(300, windowDimensions[1] / 2 + 20));
    }
    else {
      ImGui::SetNextWindowPos(ImVec2(20, 20));
      ImGui::SetNextWindowSize(ImVec2(300, windowDimensions[1] - 40));
    }

    ImGui::Begin("stats");

    double nowTime = glfwGetTime();
    deltaTime = nowTime - lastTime;
    lastTime = nowTime;

    // Handle inputs
    {
      float speed = movementSpeed * (keys[GLFW_KEY_LEFT_SHIFT] ? 15000.0 : 5000.0) * deltaTime;
      if (keys[GLFW_KEY_W]) {
        camera->ProcessKeyboard(Camera_Movement::FORWARD, speed);
        //camera->translate(0.0f, 0.0f, 10.0f);
      }

      if (keys[GLFW_KEY_S]) {
        camera->ProcessKeyboard(Camera_Movement::BACKWARD, speed);
      }

      if (keys[GLFW_KEY_D]) {
        camera->ProcessKeyboard(Camera_Movement::RIGHT, speed);
      }

      if (keys[GLFW_KEY_A]) {
        camera->ProcessKeyboard(Camera_Movement::LEFT, speed);
      }

      if (keys[GLFW_KEY_TAB]) {
        debug = 1.0f;
      }
      else {
        debug = 0.0f;
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
        glfwGetWindowSize(window, &windowDimensions[0], &windowDimensions[1]);
        fbo = new FBO(windowDimensions[0], windowDimensions[1]);

      }

      // mouse look like free-fly/fps
      double xpos, ypos, delta[2];
      glfwGetCursorPos(window, &xpos, &ypos);
      delta[0] = xpos - mouse[0];
      delta[1] = mouse[1] - ypos;
      mouse[0] = xpos;
      mouse[1] = ypos;

      camera->ProcessMouseMovement((float)delta[0], (float)delta[1]);
    }

    // Handle gamepad input
    {
      GLFWgamepadstate state;
      if (glfwGetGamepadState(GLFW_JOYSTICK_1, &state)) {
        int axis_count;
        const float x = state.axes[GLFW_GAMEPAD_AXIS_RIGHT_X];
        const float y = state.axes[GLFW_GAMEPAD_AXIS_LEFT_Y];
        const float z = state.axes[GLFW_GAMEPAD_AXIS_RIGHT_Y];

        glm::vec3 move(
          fabs(x) > 0.1 ? x * 100 * deltaTime : 0.0,
          fabs(y) > 0.1 ? -y * 100 * deltaTime : 0.0,
          fabs(z) > 0.1 ? -z * 100 * deltaTime : 0.0
        );


        if (fabs(x) > 0.1 || fabs(z) > 0.1) {
          float dir = atan2(-move.x, -move.z);

        }
      }
    }

    viewMatrix = camera->GetViewMatrix();

    MVP = perspectiveMatrix * viewMatrix;

    glm::mat4 invertedView = glm::inverse(viewMatrix);
    glm::vec3 currentEye(invertedView[3][0], invertedView[3][1], invertedView[3][2]);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glViewport(0, 0, windowDimensions[0], windowDimensions[1]);
    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LESS);
    glDepthMask(GL_TRUE);

    glCullFace(GL_BACK);
    glEnable(GL_CULL_FACE);

    glClearDepth(1.0);
    glClearColor(0.0, 0.0, 0.0, 1.0);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    glm::mat4 VP = perspectiveMatrix * viewMatrix;

    static Program *voxelProgram = Program::New()
      ->add(Shaders::get("mesh.vert"))
      ->add(Shaders::get("mesh.frag"))
      ->output("outColor")
      ->link();


    voxelProgram->use()
      ->uniformMat4("mvp", VP)
      ->uniform1ui("totalTriangles", cylMesh->faces.size())
      ->uniform1ui("totalVerts", cylMesh->verts.size());

    cylMesh->render(voxelProgram, "position");



    // Stats
    {
      static float f = 0.0f;
      static int counter = 0;
      ImGui::Text("%.1fFPS (%.3f ms)", ImGui::GetIO().Framerate, 1000.0f / ImGui::GetIO().Framerate);
      ImGui::Text("camera(%.3f, %.3f, %.3f)", camera->Position.x, camera->Position.y, camera->Position.z);
      ImGui::End();

      if (!shaderLogs.empty()) {
        ImGui::SetNextWindowPos(ImVec2(0, 0));
        ImGui::SetNextWindowSize(ImVec2(windowDimensions[0], windowDimensions[1] / 2));
        ImGui::Begin(
          "Shader Errors",
          &yes,
          ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoTitleBar
          );
        for (auto const& log : shaderLogs) {
          unsigned int lines = std::count(log.second.begin(), log.second.end(), '\n');
          ImGui::BeginChild(log.first.c_str(), ImVec2(0, 20 + 20*lines), true);
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

    time++;

    uv_run(uv_default_loop(), UV_RUN_NOWAIT);
    glfwPollEvents();
  }

  std::cout << "SHUTING DOWN" << endl;

  delete fullscreen_surface;
  Shaders::destroy();
  uv_stop(uv_default_loop());
  uv_loop_close(uv_default_loop());
  glfwTerminate();
  return 0;
}
