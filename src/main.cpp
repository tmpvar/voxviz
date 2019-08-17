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
#include "splats.h"

#define _USE_MATH_DEFINES
#include <math.h>
#include <sstream>
#include <string.h>
#include <queue>
#include <map>
#include <chrono>

#undef min
#undef max

#include <openvdb/openvdb.h>
#include <openvdb/tools/LevelSetSphere.h>
#include <openvdb/tools/LevelSetPlatonic.h>
#include <openvdb/tools/Composite.h>
#include <openvdb/tools/GridTransformer.h>
#include <tbb/tbb.h>
#include <openvr.h>

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

string GetTrackedDeviceClassString(vr::ETrackedDeviceClass td_class) {

  string str_td_class = "Unknown class";

  switch (td_class)
  {
  case vr::TrackedDeviceClass_Invalid:			// = 0, the ID was not valid.
    str_td_class = "invalid";
    break;
  case vr::TrackedDeviceClass_HMD:				// = 1, Head-Mounted Displays
    str_td_class = "hmd";
    break;
  case vr::TrackedDeviceClass_Controller:			// = 2, Tracked controllers
    str_td_class = "controller";
    break;
  case vr::TrackedDeviceClass_GenericTracker:		// = 3, Generic trackers, similar to controllers
    str_td_class = "generic tracker";
    break;
  case vr::TrackedDeviceClass_TrackingReference:	// = 4, Camera and base stations that serve as tracking reference points
    str_td_class = "base station";
    break;
  case vr::TrackedDeviceClass_DisplayRedirect:	// = 5, Accessories that aren't necessarily tracked themselves, but may redirect video output from other tracked devices
    str_td_class = "display redirect";
    break;
  }

  return str_td_class;
}

vr::IVRSystem* vr_context;
vr::TrackedDevicePose_t tracked_device_pose[vr::k_unMaxTrackedDeviceCount];
string tracked_device_type[vr::k_unMaxTrackedDeviceCount];

string GetTrackedDeviceString(vr::IVRSystem *pHmd, vr::TrackedDeviceIndex_t unDevice, vr::TrackedDeviceProperty prop, vr::TrackedPropertyError *peError)
{
  uint32_t requiredBufferLen = pHmd->GetStringTrackedDeviceProperty(unDevice, prop, NULL, 0, peError);
  if (requiredBufferLen == 0)
    return "";

  char *pchBuffer = new char[requiredBufferLen];
  requiredBufferLen = pHmd->GetStringTrackedDeviceProperty(unDevice, prop, pchBuffer, requiredBufferLen, peError);
  string sResult = pchBuffer;
  delete[] pchBuffer;

  return sResult;
}


bool vrcontrollerButtons[255];
bool triggerDown = false;
void process_vr_event(const vr::VREvent_t & event)
{
  string str_td_class = GetTrackedDeviceClassString(vr_context->GetTrackedDeviceClass(event.trackedDeviceIndex));

  switch (event.eventType)
  {
  case vr::VREvent_TrackedDeviceActivated:
  {
    cout << "Device " << event.trackedDeviceIndex << " attached (" << str_td_class << ")" << endl;
    tracked_device_type[event.trackedDeviceIndex] = str_td_class;
  }
  break;
  case vr::VREvent_TrackedDeviceDeactivated:
  {
    cout << "Device " << event.trackedDeviceIndex << " detached (" << str_td_class << ")" << endl;
    tracked_device_type[event.trackedDeviceIndex] = "";
  }
  break;
  case vr::VREvent_TrackedDeviceUpdated:
  {
    cout << "Device " << event.trackedDeviceIndex << " updated (" << str_td_class << ")" << endl;
  }
  break;
  case vr::VREvent_ButtonPress:
  {
    vr::VREvent_Controller_t controller_data = event.data.controller;
    cout << "Pressed button " << vr_context->GetButtonIdNameFromEnum((vr::EVRButtonId) controller_data.button) << " of device " << event.trackedDeviceIndex << " (" << str_td_class << ")" << endl;

    if (controller_data.button == vr::k_EButton_Axis1) {
      triggerDown = true;
    }
  }
  break;
  case vr::VREvent_ButtonUnpress:
  {
    vr::VREvent_Controller_t controller_data = event.data.controller;
    cout << "Unpressed button " << vr_context->GetButtonIdNameFromEnum((vr::EVRButtonId) controller_data.button) << " of device " << event.trackedDeviceIndex << " (" << str_td_class << ")" << endl;
    if (controller_data.button == vr::k_EButton_Axis1) {
      triggerDown = false;
    }
  }
  break;
  case vr::VREvent_ButtonTouch:
  {
    vr::VREvent_Controller_t controller_data = event.data.controller;
    cout << "Touched button " << vr_context->GetButtonIdNameFromEnum((vr::EVRButtonId) controller_data.button) << " of device " << event.trackedDeviceIndex << " (" << str_td_class << ")" << endl;
  }
  break;
  case vr::VREvent_ButtonUntouch:
  {
    vr::VREvent_Controller_t controller_data = event.data.controller;
    cout << "Untouched button " << vr_context->GetButtonIdNameFromEnum((vr::EVRButtonId) controller_data.button) << " of device " << event.trackedDeviceIndex << " (" << str_td_class << ")" << endl;
  }

  break;
  }
}

// lifted from https://github.com/highfidelity/hifi/blob/master/plugins/openvr/src/OpenVrHelpers.h
inline mat4 toGlm(const vr::HmdMatrix34_t& m) {
  mat4 result = mat4(
    m.m[0][0], m.m[1][0], m.m[2][0], 0.0,
    m.m[0][1], m.m[1][1], m.m[2][1], 0.0,
    m.m[0][2], m.m[1][2], m.m[2][2], 0.0,
    m.m[0][3], m.m[1][3], m.m[2][3], 1.0f);
  return result;
}

SplatBuffer *fillSplatBuffer(SplatBuffer *buf, openvdb::FloatGrid::Ptr grid) {
  buf->splat_count = 0;
  using GridType = openvdb::FloatGrid;
  using TreeType = GridType::TreeType;
  Splat *data = (Splat *)buf->ssbo->beginMap(SSBO::MAP_WRITE_ONLY);
  for (GridType::ValueOnCIter iter = grid->cbeginValueOn(); iter.test(); ++iter) {
    openvdb::Vec3d c = grid->transform().indexToWorld(iter.getCoord().asVec3d());
    data[buf->splat_count].position = glm::vec4(c.x(), c.y(), c.z(), 1.0);
    buf->splat_count++;
  }
  buf->ssbo->endMap();
  return buf;
}

int main(void) {

  openvdb::initialize();


  openvdb::FloatGrid::Ptr toolGrid =
    openvdb::tools::createLevelSetSphere<openvdb::FloatGrid>(
      /*radius=*/50.0, /*center=*/openvdb::Vec3f(1.5, 2, 3),
      /*voxel size=*/1, /*width=*/1.1);

  openvdb::math::Transform::Ptr toolIdentity = openvdb::math::Transform::createLinearTransform(openvdb::Mat4R::identity());
  toolGrid->setTransform(toolIdentity);
  /*
  openvdb::math::BBox<openvdb::Vec3d> worldGridBounds(
    openvdb::Vec3d(-200, -5, -200),
    openvdb::Vec3d(100, 5, 100)
  );

  openvdb::math::Transform::Ptr worldIdentity = openvdb::math::Transform::createLinearTransform(openvdb::Mat4R::identity());

  openvdb::FloatGrid::Ptr worldGrid = openvdb::tools::createLevelSetBox<openvdb::FloatGrid>(
    worldGridBounds,
    *worldIdentity,
    1.1f
  );
  */

  //openvdb::FloatGrid::Ptr worldGrid = openvdb::tools::createLevelSetSphere<openvdb::FloatGrid>(
  //  /*radius=*/50.0, /*center=*/openvdb::Vec3f(1.5, 2, 3),
  //  /*voxel size=*/1, /*width=*/1.1);

  openvdb::FloatGrid::Ptr worldGrid = openvdb::FloatGrid::create(1000.0f);

  openvdb::math::Transform::Ptr worldIdentity = openvdb::math::Transform::createLinearTransform(openvdb::Mat4R::identity());
  worldGrid->setTransform(toolIdentity);

  if (!vr::VR_IsHmdPresent() || !vr::VR_IsRuntimeInstalled) {
    return -1;
  }

  cout << "found hmd and openvr runtime!" << endl;
  vr::HmdError err;
  vr_context = vr::VR_Init(&err, vr::EVRApplicationType::VRApplication_Scene);
  vr_context == NULL ? cout << "Error while initializing SteamVR runtime. Error code is " << vr::VR_GetVRInitErrorAsSymbol(err) << endl : cout << "SteamVR runtime successfully initialized" << endl;

  memset(keys, 0, sizeof(keys));

  int d = BRICK_DIAMETER;
  float fd = (float)d;
  int dims[3] = { d, d, d };
  float dsquare = (float)d*(float)d;
  float camera_z = sqrtf(dsquare * 3.0f) * 1.5f;
  GLFWgamepadstate state;
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
  if (glDebugMessageCallback) {
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

  Raytracer *raytracer = new Raytracer(dims);
  float max_distance = 10000.0f;

  glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
  glfwGetCursorPos(window, &mouse[0], &mouse[1]);
  FreeCamera *camera = new FreeCamera(
    glm::vec3(1, -1, -1),
	glm::vec3(0.0, 1.0, 0.0),
	90.0,
	0.0
  );

  unsigned int time = 0;

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

  FullscreenSurface *fullscreen_surface = new FullscreenSurface();
  fbo = new FBO(windowDimensions[0], windowDimensions[1]);


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

  uint64_t total_light_slab_slots = total_voxel_slab_slots;
  SSBO *lightSpaceSSBO = new SSBO(total_light_slab_slots * uint64_t(16));

  #define MAX_LIGHTS 4
  SSBO *lightBuffer = new SSBO(
    sizeof(Light) * MAX_LIGHTS // light position + color,intensity
  );

  uint64_t outputBytes =
    static_cast<uint64_t>(windowDimensions[0]) *
    static_cast<uint64_t>(windowDimensions[1]) * 16;

  raytraceOutput = new SSBO(outputBytes);
  uint64_t terminationBytes =
    static_cast<uint64_t>(windowDimensions[0]) *
    static_cast<uint64_t>(windowDimensions[1]) * sizeof(RayTermination);

  #define TAA_HISTORY_LENGTH 1
  terminationOutput = new SSBO(terminationBytes * static_cast<uint64_t>(TAA_HISTORY_LENGTH));

  cout << "window dimensions: " << windowDimensions[0] << ", " << windowDimensions[1] << endl;

  Program *raytraceVoxelSpace = new Program();
  raytraceVoxelSpace
    ->add(Shaders::get("voxel-space-raytrace.vert"))
    ->add(Shaders::get("voxel-space-raytrace.frag"))
    ->output("outColor")
    ->link();

  Program *debugBindless = new Program();
  debugBindless
    ->add(Shaders::get("basic.vert"))
    ->add(Shaders::get("debug-bindless.frag"))
    ->output("outColor")
    ->link();

  BlueNoise1x64x64x64 *blue_noise = new BlueNoise1x64x64x64();
  blue_noise->upload();

  VoxEntity *catModel = nullptr;

  vector <SplatBuffer *>splatBuffers;

  SplatBuffer *toolBuffer = new SplatBuffer();//new SSBO(sizeof(Splat) * 1 << 25);
  splatBuffers.push_back(toolBuffer);

  SplatBuffer *worldBuffer = new SplatBuffer();//new SSBO(sizeof(Splat) * 1 << 25);
  splatBuffers.push_back(worldBuffer);

  // Collect VDB splats
  {
    fillSplatBuffer(toolBuffer, toolGrid);
    fillSplatBuffer(worldBuffer, worldGrid);
  }


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

    // Shader logs
    {
      if (!shaderLogs.empty()) {
        ImGui::SetNextWindowPos(ImVec2(20, windowDimensions[1] / 2 + 20));
        ImGui::SetNextWindowSize(ImVec2(300, windowDimensions[1] / 2 + 20));
      }
      else {
        ImGui::SetNextWindowPos(ImVec2(20, 20));
        ImGui::SetNextWindowSize(ImVec2(300, windowDimensions[1] - 40));
      }
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

      if (triggerDown || keys[GLFW_KEY_SPACE] || state.axes[GLFW_GAMEPAD_AXIS_RIGHT_TRIGGER] > 0.1) {
        using GridType = openvdb::FloatGrid;
        using TreeType = GridType::TreeType;

        const openvdb::math::Transform
          &toolXform = toolGrid->transform(),
          &worldXform = worldGrid->transform();

        openvdb::tools::GridTransformer transformer(
          toolXform.baseMap()->getAffineMap()->getMat4()// *
          //worldXform.baseMap()->getAffineMap()->getMat4().inverse()
        );

        glm::vec3 toolPos = toolBuffer->model[3];

        openvdb::tools::GridTransformer transformer2(
          openvdb::Vec3R(0.0, 0.0, 0.0),
          openvdb::Vec3R(1.0, 1.0, 1.0),
          openvdb::Vec3R(0.0, 0.0, 0.0),
          openvdb::Vec3R(toolPos.x, toolPos.y, toolPos.z)
        );

        openvdb::FloatGrid::Ptr tmpGrid = openvdb::FloatGrid::create(
          toolGrid->background()
        );

        transformer2.transformGrid<openvdb::tools::BoxSampler, openvdb::FloatGrid>(
          *toolGrid,
          *tmpGrid
        );

        openvdb::tools::csgUnion(*worldGrid, *tmpGrid);
        //openvdb::tools::compMin(*worldGrid, *tmpGrid);
        worldGrid->tree().prune();

        fillSplatBuffer(worldBuffer, worldGrid);
      }

      if (keys[GLFW_KEY_H]) {
        raytracer->showHeat = 1;
      }
      else {
        raytracer->showHeat = 0;
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
      if (glfwGetGamepadState(GLFW_JOYSTICK_1, &state)) {
        int axis_count;
        const float x = state.axes[GLFW_GAMEPAD_AXIS_RIGHT_X];
        const float y = state.axes[GLFW_GAMEPAD_AXIS_LEFT_Y];
        const float z = state.axes[GLFW_GAMEPAD_AXIS_RIGHT_Y];

        glm::vec3 move(
          fabs(x) > 0.1 ? -x * 100 * deltaTime : 0.0,
          fabs(y) > 0.1 ? -y * 100 * deltaTime : 0.0,
          fabs(z) > 0.1 ? -z * 100 * deltaTime : 0.0
        );

        toolBuffer->model = glm::translate(toolBuffer->model, move);
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

    // OpenVR Debug
    {
      if (vr_context != NULL) {
        // Process SteamVR events
        vr::VREvent_t vr_event;
        while (vr_context->PollNextEvent(&vr_event, sizeof(vr_event))) {
          process_vr_event(vr_event);
        }

        // Obtain tracking device poses
        vr_context->GetDeviceToAbsoluteTrackingPose(vr::ETrackingUniverseOrigin::TrackingUniverseSeated, 0, tracked_device_pose, vr::k_unMaxTrackedDeviceCount);
        vr::VRControllerState_t controllerState;
        int actual_y = 110, tracked_device_count = 0;

        int controller = 2;
        for (int nDevice = 0; nDevice < vr::k_unMaxTrackedDeviceCount; nDevice++)
        {
          if ((tracked_device_pose[nDevice].bDeviceIsConnected) && (tracked_device_pose[nDevice].bPoseIsValid))
          {
            // Check whether the tracked device is a controller. If so, set text color based on the trigger button state
            vr::VRControllerState_t controller_state;

            // We take just the translation part of the matrix (actual position of tracked device, not orientation)
            float v[3] = {
              tracked_device_pose[nDevice].mDeviceToAbsoluteTracking.m[0][3],
              tracked_device_pose[nDevice].mDeviceToAbsoluteTracking.m[1][3],
              tracked_device_pose[nDevice].mDeviceToAbsoluteTracking.m[2][3]
            };


            ImGui::Text("device #%i (%s)\n   pos (%.3f, %.3f, %.3f)", nDevice, tracked_device_type[nDevice].c_str(), v[0], v[1], v[2]);


            if (nDevice == controller) {
              static glm::vec3 initialPosition = glm::vec3(
                tracked_device_pose[controller].mDeviceToAbsoluteTracking.m[0][3],
                tracked_device_pose[controller].mDeviceToAbsoluteTracking.m[1][3],
                tracked_device_pose[controller].mDeviceToAbsoluteTracking.m[2][3]
              );

              tracked_device_pose[controller].mDeviceToAbsoluteTracking.m[0][3] -= initialPosition.x;
              tracked_device_pose[controller].mDeviceToAbsoluteTracking.m[1][3] -= initialPosition.y;
              tracked_device_pose[controller].mDeviceToAbsoluteTracking.m[2][3] -= initialPosition.z;

              toolBuffer->model = toGlm(tracked_device_pose[controller].mDeviceToAbsoluteTracking);
              toolBuffer->model[3] *= glm::vec4(1000.0, 1000.0, 1000.0, 1.0);
            }


            tracked_device_count++;
          }
        }

        ImGui::Text("devices: %i", tracked_device_count);
      }
    }

    // Splats
    if (true) {
      glm::uvec2 resolution(windowDimensions[0], windowDimensions[1]);
      // Raster with GL_POINT
      if (!debug) {
        static Program* rasterSplats = Program::New()
          ->add(Shaders::get("splats/raster.vert"))
          ->add(Shaders::get("splats/raster.frag"))
          ->output("outColor")
          ->link();

        // Raster splats using draw arrays

        rasterSplats
          ->use()

          ->uniformVec3("eye", currentEye)
          ->uniformFloat("fov", fov)
          ->uniformVec2ui("res", resolution)
          ->uniform1ui("mipLevel", 0);

        glEnable(GL_PROGRAM_POINT_SIZE);
        size_t total_splats = 0;
        for (auto &buffer : splatBuffers) {
          GLuint query;
          GLuint64 elapsed_time;
          GLint done = 0;
          glGenQueries(1, &query);
          glBeginQuery(GL_TIME_ELAPSED, query);

          total_splats += buffer->splat_count;
          rasterSplats
            ->ssbo("splatInstanceBuffer", buffer->ssbo)
            ->uniform1ui("maxSplats", buffer->max_splats)
            ->uniformMat4("mvp", perspectiveMatrix * viewMatrix * buffer->model);
          glDrawArrays(GL_POINTS, 0, buffer->splat_count);

          glEndQuery(GL_TIME_ELAPSED);
          while (!done) {
            glGetQueryObjectiv(query, GL_QUERY_RESULT_AVAILABLE, &done);
          }

          // get the query result
          glGetQueryObjectui64v(query, GL_QUERY_RESULT, &elapsed_time);
          ImGui::Text("gl_points: %.3f.ms", elapsed_time / 1000000.0);
          glDeleteQueries(1, &query);
        }

        ImGui::Text("splats: %lu", total_splats);
        gl_error();
      }

      if (true || debug) {
        static Program* rasterSplats_Compute = Program::New()
          ->add(Shaders::get("splats/raster.comp"))
          ->link();
        static SSBO* pixelBuffer = new SSBO(0);

        pixelBuffer->resize(
          8 * resolution.x * resolution.y,
          glm::vec3(resolution.x, resolution.y, 0)
        );

        size_t total_splats = 0;
        for (auto &buffer : splatBuffers) {
          total_splats += buffer->splat_count;

          rasterSplats_Compute->use()
            ->uniformVec2ui("resolution", resolution)
            ->uniformMat4("MVP", MVP)
            ->ssbo("splatInstanceBuffer", buffer->ssbo)
            ->ssbo("pixelBuffer", pixelBuffer)
            ->timedCompute("splats compute", glm::uvec3(buffer->splat_count, 1, 1));

          glDrawArrays(GL_POINTS, 0, buffer->splat_count);
        }
      }
    }

    // Frame stats
    {
      static float f = 0.0f;
      static int counter = 0;
      ImGui::Text("%.1fFPS (%.3f ms)", ImGui::GetIO().Framerate, 1000.0f / ImGui::GetIO().Framerate);
      ImGui::Text("camera(%.3f, %.3f, %.3f)", camera->Position.x, camera->Position.y, camera->Position.z);
      if (catModel != nullptr) {
        ImGui::Text("cat(%.3f, %.3f, %.3f)", catModel->getPosition().x, catModel->getPosition().y, catModel->getPosition().z);
      }
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

  delete fullscreen_program;
  delete fullscreen_surface;
  delete raytracer;
  Shaders::destroy();
  uv_stop(uv_default_loop());
  uv_loop_close(uv_default_loop());
  glfwTerminate();
  return 0;
}
