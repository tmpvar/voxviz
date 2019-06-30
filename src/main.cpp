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

bool keys[1024];
bool prevKeys[1024];
double mouse[2];
bool fullscreen = 0;
bool yes = true;
// int windowDimensions[2] = { 1024, 768 };
int windowDimensions[2] = { 1440, 900 };
glm::mat4 viewMatrix, perspectiveMatrix, MVP;
FBO *fbo = nullptr;

SSBO *raytraceOutput = nullptr;
#define TAA_HISTORY_LENGTH 16
#define RAY_TERMINATION_BYTES 64
#define MAX_MIP_LEVELS 7
SSBO *terminationOutput = nullptr;
typedef struct Light {
  glm::vec4 position;
  glm::vec4 color;
};


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
      static_cast<uint64_t>(height) * RAY_TERMINATION_BYTES;
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
      GL_DONT_CARE,
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
    glm::vec3(100, 100, -200),
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
  const float movementSpeed = 0.01f;
  GLFWgamepadstate state;
  
  // Voxel space
  const glm::uvec3 voxelSpaceDims = glm::uvec3(1024, 256, 128);
   
  uint64_t voxelSpaceBytes =
    static_cast<uint64_t>(voxelSpaceDims.x) *
    static_cast<uint64_t>(voxelSpaceDims.y) *
    static_cast<uint64_t>(voxelSpaceDims.z);

  uint64_t total_voxel_slab_slots = 0;
  for (unsigned int i = 0; i <= MAX_MIP_LEVELS; i++) {
    uint64_t a = voxelSpaceBytes >> (i * 3);
    total_voxel_slab_slots += a;
  }

  SSBO *voxelSpaceSSBO = new SSBO(total_voxel_slab_slots /* 1 byte per voxel.. for now */);

  uint64_t total_light_slab_slots = total_voxel_slab_slots;
  SSBO *lightSpaceSSBO = new SSBO(total_light_slab_slots * uint64_t(16));
  
  #define MAX_LIGHTS 4
  SSBO *lightBuffer = new SSBO(
    16 * MAX_LIGHTS // light position + color,intensity
  );

  Program *fillVoxelSpace = new Program();
  fillVoxelSpace->add(Shaders::get("voxel-space-fill.comp"))->link();

  Program *fillVoxelSpaceSDF = new Program();
  fillVoxelSpaceSDF->add(Shaders::get("voxel-space-fill-sdf.comp"))->link();

  Program *clearVoxelSpaceSDF = new Program();
  clearVoxelSpaceSDF->add(Shaders::get("voxel-space-clear-sdf.comp"))->link();

  Program *mipmapVoxelSpace = new Program();
  mipmapVoxelSpace->add(Shaders::get("voxel-space-mipmap.comp"))->link();

  Program *raytraceVoxelSpace_Compute = new Program();
  raytraceVoxelSpace_Compute->add(Shaders::get("voxel-space-raytrace.comp"))->link();
  
  //Program *skyRays_Compute = new Program();
  //skyRays_Compute->add(Shaders::get("voxel-space-sky-rays.comp"))->link();

  Program *raytraceVoxelSpace_Blur = new Program();
  raytraceVoxelSpace_Blur->add(Shaders::get("voxel-space-blur.comp"))->link();

  //Program *gravityVoxelSpace = new Program();
  //gravityVoxelSpace->add(Shaders::get("voxel-space-gravity.comp"))->link();

  //Program *lightRays_Compute = new Program();
  //lightRays_Compute->add(Shaders::get("voxel-space-conetrace-lights.comp"))->link();

  Program *lightSpaceClear_Compute = new Program();
  lightSpaceClear_Compute->add(Shaders::get("light-space-clear.comp"))->link();

  Program *lightSpaceFill_Compute = new Program();
  lightSpaceFill_Compute->add(Shaders::get("light-space-fill.comp"))->link();

  Program *lightSpaceBlurUp_Compute = new Program();
  lightSpaceBlurUp_Compute->add(Shaders::get("light-space-blur-up.comp"))->link();

  Program *lightSpaceBlurDown_Compute = new Program();
  lightSpaceBlurDown_Compute->add(Shaders::get("light-space-blur-down.comp"))->link();


  Program *mipmapLightSpace = new Program();
  mipmapLightSpace->add(Shaders::get("light-space-mipmap.comp"))->link();


  Program *lightSpaceConeTrace_Compute = new Program();
  lightSpaceConeTrace_Compute->add(Shaders::get("light-space-conetrace.comp"))->link();
  
  uint64_t outputBytes =
    static_cast<uint64_t>(windowDimensions[0]) *
    static_cast<uint64_t>(windowDimensions[1]) * 16;

  raytraceOutput = new SSBO(outputBytes);
  uint64_t terminationBytes =
    static_cast<uint64_t>(windowDimensions[0]) *
    static_cast<uint64_t>(windowDimensions[1]) * RAY_TERMINATION_BYTES;

  #define TAA_HISTORY_LENGTH 1
  terminationOutput = new SSBO(terminationBytes * static_cast<uint64_t>(TAA_HISTORY_LENGTH));

  cout << "window dimensions: " << windowDimensions[0] << ", " << windowDimensions[1] << endl;
  // Draw a plane under the scene
  {
    fillVoxelSpace
      ->use()
      ->ssbo("volumeSlabBuffer", voxelSpaceSSBO)
      ->uniformVec3ui("volumeSlabDims", voxelSpaceDims)

      ->uniform1ui("time", time)
      ->compute(voxelSpaceDims);

    gl_error();
  }

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

  VoxEntity *catModel;
  // Load vox models
  {
    catModel = new VoxEntity(
      "D:\\work\\voxel-model\\vox\\character\\chr_cat.vox",
      glm::vec3(200.0, 50.0, 10.0)
    );

    VoxEntity *voxMonu1 = new VoxEntity(
      "D:\\work\\voxel-model\\vox\\monument\\monu1.vox",
      glm::uvec3(300, 64, 100)
    );

    VoxEntity *voxMonu2 = new VoxEntity(
      "D:\\work\\voxel-model\\vox\\monument\\monu2.vox",
      glm::uvec3(200, 64, 300)
    );

    VoxEntity *voxMonu3 = new VoxEntity(
      "D:\\work\\voxel-model\\vox\\monument\\monu3.vox",
      glm::uvec3(200, 64, 100)
    );


    VoxEntity *voxMonu4 = new VoxEntity(
      "D:\\work\\voxel-model\\vox\\monument\\monu4.vox",
      glm::uvec3(0, 64, 100)
    );

    VoxEntity *voxMonu5 = new VoxEntity(
      "D:\\work\\voxel-model\\vox\\monument\\monu5.vox",
      glm::uvec3(100, 64, 0)
    );

    VoxEntity *voxMonu6 = new VoxEntity(
      "D:\\work\\voxel-model\\vox\\monument\\monu6.vox",
      glm::uvec3(100, 64, 100)
    );


    VoxEntity *voxMonu7 = new VoxEntity(
      "D:\\work\\voxel-model\\vox\\monument\\monu16.vox",
      glm::uvec3(300, 64, 100)
    );


    VoxEntity *voxModelFirst = new VoxEntity(
      "D:\\work\\voxviz\\img\\models\\first.vox",
      glm::uvec3(128, 100, 200)
    );



    VoxEntity *voxModelRh2house = new VoxEntity(
      "D:\\work\\voxviz\\img\\models\\rh2house.vox",
      glm::uvec3(50, 64, 100)
    );

    uint8_t *buf = (uint8_t *)voxelSpaceSSBO->beginMap(SSBO::MAP_WRITE_ONLY);

    catModel->paintInto(buf, voxelSpaceDims);
    voxMonu1->paintInto(buf, voxelSpaceDims);
    voxMonu2->paintInto(buf, voxelSpaceDims);
    voxMonu3->paintInto(buf, voxelSpaceDims);
    voxMonu4->paintInto(buf, voxelSpaceDims);
    voxMonu5->paintInto(buf, voxelSpaceDims);
    voxMonu6->paintInto(buf, voxelSpaceDims);
    voxMonu7->paintInto(buf, voxelSpaceDims);
    
    voxModelFirst->paintInto(buf, voxelSpaceDims);
    voxModelRh2house->paintInto(buf, voxelSpaceDims);

    voxelSpaceSSBO->endMap();
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
        camera->ProcessKeyboard(Camera_Movement::LEFT, speed);
      }

      if (keys[GLFW_KEY_A]) {
        camera->ProcessKeyboard(Camera_Movement::RIGHT, speed);
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
      delta[0] = mouse[0] - xpos;
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
        catModel->move(move);
        catModel->clampPosition(glm::vec3(8.0, 16, 0.0), glm::vec3(voxelSpaceDims) - glm::vec3(8.0, 0.0, 16.0));
        

        if (fabs(x) > 0.1 || fabs(z) > 0.1) {
          float dir = atan2(-move.x, -move.z);

          catModel->setRotation(glm::vec3(
            0.0,
            dir,
            0.0
          ));
        }

        /*lastCharacterPos = characterPos;
        characterPos += glm::vec3(
          fabs(x) > 0.1 ? x * 100 * deltaTime : 0,
          fabs(y) > 0.1 ? y * 100 * deltaTime : 0,
          fabs(z) > 0.1 ? -z * 100 * deltaTime : 0
        );
        */
        
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

    // Regenerate world
    if (true) {
      // Clear the voxel space volume
      glm::uvec3 sdfDims(40, 40, 40);

      clearVoxelSpaceSDF
        ->use()
        ->ssbo("volumeSlabBuffer", voxelSpaceSSBO)
        ->uniformVec3ui("volumeSlabDims", voxelSpaceDims)

        ->uniform1ui("time", lastCharacterTime)
        ->uniformVec3("offset", lastCharacterPos)
        ->uniformVec3ui("sdfDims", sdfDims)
        ->timedCompute("sdf: clear", sdfDims);
      // disable glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);

      // Fill the voxel space volume
      fillVoxelSpaceSDF
        ->use()
        ->ssbo("volumeSlabBuffer", voxelSpaceSSBO)
        ->uniformVec3ui("volumeSlabDims", voxelSpaceDims)

        ->uniform1ui("time", time)
        ->uniformVec3("offset", characterPos)
        ->uniformVec3ui("sdfDims", sdfDims)
        ->timedCompute("sdf: fill", sdfDims);

      lastCharacterTime = time;

      uint8_t *buf = (uint8_t *)voxelSpaceSSBO->beginMap(SSBO::MAP_WRITE_ONLY);
      catModel->paintInto(buf, voxelSpaceDims);
      voxelSpaceSSBO->endMap();
      

      gl_error();
      glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);
    }
    
    // Generate mipmaps
    if (true) {
      double mipStart = glfwGetTime();
      // Generate mipmap for SSBO
      for (unsigned int i = 1; i <= MAX_MIP_LEVELS; i++) {
        glm::uvec3 mipDims = voxelSpaceDims / (glm::uvec3(1<<i));
        glm::uvec3 lowerMipDims = voxelSpaceDims / (glm::uvec3(1 << (i - 1)));
        ostringstream mipDebug;
        mipDebug << "mip " << i << " dims: " << mipDims.x << ","  << mipDims.y << "," << mipDims.z;

        mipmapVoxelSpace
          ->use()
          ->ssbo("volumeSlabBuffer", voxelSpaceSSBO)
          ->uniformVec3("volumeSlabDims", voxelSpaceDims)

          ->uniformVec3ui("mipDims", mipDims)
          ->uniformVec3ui("lowerMipDims", lowerMipDims)
          ->uniform1ui("mipLevel", i)
          ->timedCompute(mipDebug.str().c_str(), mipDims);
        gl_error();
        // disable glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);
      }
    }

    // Light space
    { 
	  {
		glm::uvec3 dims = voxelSpaceDims / glm::uvec3(2) + glm::uvec3(2);
		// this appears to be .1ms faster on my 1080ti
		lightSpaceClear_Compute
		  ->use()
		  ->ssbo("lightSlabBuffer", lightSpaceSSBO)
		  ->uniformVec3("lightSlabDims", voxelSpaceDims)
		  ->timedCompute("lightspace: clear", glm::uvec3(total_light_slab_slots, 1, 1));

		float samples = 256;
		lightSpaceFill_Compute
		  ->use()
		  ->ssbo("volumeSlabBuffer", voxelSpaceSSBO)
		  ->uniformVec3("volumeSlabDims", voxelSpaceDims)

		  ->ssbo("lightSlabBuffer", lightSpaceSSBO)
		  ->uniformVec3("lightSlabDims", voxelSpaceDims)

		  ->ssbo("blueNoiseBuffer", blue_noise->ssbo)
		  ->uniform1ui("time", time)
		  ->uniformVec3("lightPos", catModel->getPosition() + glm::vec3(10.0))
		  ->uniformVec3("lightColor", glm::vec3(1.0))
		  ->uniformFloat("samples", samples);

		// axis is specified as x=0, y=1, z=2

		lightSpaceFill_Compute
		  ->uniform1ui("axis", 0)
		  ->uniform1ui("start", 0)
		  ->timedCompute("lightspace: fill -X", glm::uvec3(
			dims.y,
			dims.z,
			1
		  ));

		lightSpaceFill_Compute
		  ->uniform1ui("axis", 0)
		  ->uniform1ui("start", 1)
		  ->timedCompute("lightspace: fill +X", glm::uvec3(
			dims.y,
			dims.z,
			1
		  ));
      
		lightSpaceFill_Compute
		  ->uniform1ui("axis", 1)
		  ->uniform1ui("start", 0)
		  ->timedCompute("lightspace: fill -Y", glm::uvec3(
			dims.x,
			dims.z,
			1
		  ));

		lightSpaceFill_Compute
		  ->uniform1ui("axis", 1)
		  ->uniform1ui("start", 1)
		  ->timedCompute("lightspace: fill +Y", glm::uvec3(
			dims.x,
			dims.z,
			1
		  ));

		lightSpaceFill_Compute
		  ->uniform1ui("axis", 2)
		  ->uniform1ui("start", 0)
		  ->timedCompute("lightspace: fill -Z", glm::uvec3(
			dims.x,
			dims.y,
			1
		  ));

		lightSpaceFill_Compute
		  ->uniform1ui("axis", 2)
		  ->uniform1ui("start", 1)
		  ->timedCompute("lightspace: fill +Z", glm::uvec3(
			dims.x,
			dims.y,
			1
		  ));



		lightSpaceFill_Compute
		  ->use()
		  ->ssbo("volumeSlabBuffer", voxelSpaceSSBO)
		  ->uniformVec3("volumeSlabDims", voxelSpaceDims)

		  ->ssbo("lightSlabBuffer", lightSpaceSSBO)
		  ->uniformVec3("lightSlabDims", voxelSpaceDims)

		  ->ssbo("blueNoiseBuffer", blue_noise->ssbo)
		  ->uniform1ui("time", time)
		  ->uniformVec3("lightPos", glm::vec3(10, 50, 30))
		  ->uniformVec3("lightColor", glm::vec3(1.0, 0.0, 0.0))
		  ->uniformFloat("samples", samples);

		// axis is specified as x=0, y=1, z=2

		lightSpaceFill_Compute
		  ->uniform1ui("axis", 0)
		  ->uniform1ui("start", 0)
		  ->timedCompute("lightspace: fill -X", glm::uvec3(
			dims.y,
			dims.z,
			1
		  ));

		lightSpaceFill_Compute
		  ->uniform1ui("axis", 0)
		  ->uniform1ui("start", 1)
		  ->timedCompute("lightspace: fill +X", glm::uvec3(
			dims.y,
			dims.z,
			1
		  ));

		lightSpaceFill_Compute
		  ->uniform1ui("axis", 1)
		  ->uniform1ui("start", 0)
		  ->timedCompute("lightspace: fill -Y", glm::uvec3(
			dims.x,
			dims.z,
			1
		  ));

		lightSpaceFill_Compute
		  ->uniform1ui("axis", 1)
		  ->uniform1ui("start", 1)
		  ->timedCompute("lightspace: fill +Y", glm::uvec3(
			dims.x,
			dims.z,
			1
		  ));

		lightSpaceFill_Compute
		  ->uniform1ui("axis", 2)
		  ->uniform1ui("start", 0)
		  ->timedCompute("lightspace: fill -Z", glm::uvec3(
			dims.x,
			dims.y,
			1
		  ));

		lightSpaceFill_Compute
		  ->uniform1ui("axis", 2)
		  ->uniform1ui("start", 1)
		  ->timedCompute("lightspace: fill +Z", glm::uvec3(
			dims.x,
			dims.y,
			1
		  ));
	  }
      // disable glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);

	  lightSpaceBlurUp_Compute
		->use()
		->ssbo("lightSlabBuffer", lightSpaceSSBO)
		->uniformVec3("lightSlabDims", voxelSpaceDims);

	  for (int mip = 1; mip < MAX_MIP_LEVELS; mip++) {
		ostringstream mipDebug;
		mipDebug << "lightspace: blur up " << mip;

		glm::uvec3 mipDims = voxelSpaceDims / (glm::uvec3(1 << mip));

		lightSpaceBlurUp_Compute
		  ->uniform1ui("mipLevel", mip)
		  ->timedCompute(mipDebug.str().c_str(), mipDims);
	  }

	  glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);
	  
	  lightSpaceBlurDown_Compute
		->use()
		->ssbo("lightSlabBuffer", lightSpaceSSBO)
		->uniformVec3("lightSlabDims", voxelSpaceDims);

	  for (int mip = MAX_MIP_LEVELS - 1; mip >= 1; mip--) {
		glm::uvec3 mipDims = voxelSpaceDims / (glm::uvec3(1 << mip));
		ostringstream mipDebug;
		mipDebug << "lightspace: blur down" << mip;

		lightSpaceBlurDown_Compute
		  ->uniform1ui("mipLevel", uint32_t(mip))
		  ->timedCompute(mipDebug.str().c_str(), mipDims);
	  }

	  glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);
    }

    
    // Raytrace in compute
    if (true) {
      // Raytrace into `raytraceOutput`
      glm::uvec2 res = glm::uvec2(windowDimensions[0], windowDimensions[1]);
      {
        raytraceVoxelSpace_Compute
          ->use()

          ->uniformVec3("eye", currentEye)
          ->ssbo("volumeSlabBuffer", voxelSpaceSSBO)
          ->uniformVec3("volumeSlabDims", voxelSpaceDims)

          ->ssbo("lightSlabBuffer", lightSpaceSSBO)
          ->uniformVec3("lightSlabDims", voxelSpaceDims)

          ->ssbo("outTerminationBuffer", terminationOutput)
          ->ssbo("blueNoiseBuffer", blue_noise->ssbo)
          ->ssbo("outColorBuffer", raytraceOutput)

          ->uniformMat4("VP", VP)
          ->uniformFloat("debug", debug)
          
          ->uniformVec2ui("resolution", res)
          ->uniform1ui("terminationBufferIdx", 0)
          ->timedCompute(
            "primary rays",
            glm::uvec3(
              windowDimensions[0],
              windowDimensions[1],
              1
            )
          );
        // disable glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);
      }

      // Conetrace light buffer
      {
        lightSpaceConeTrace_Compute
          ->use()
          ->ssbo("volumeSlabBuffer", voxelSpaceSSBO)
          ->uniformVec3("volumeSlabDims", voxelSpaceDims)

          ->ssbo("outTerminationBuffer", terminationOutput)
          ->ssbo("blueNoiseBuffer", blue_noise->ssbo)

          ->ssbo("lightSlabBuffer", lightSpaceSSBO)
          ->uniformVec3("lightSlabDims", voxelSpaceDims)


          ->uniform1ui("time", time)
          ->uniformVec2ui("resolution", res)
          ->timedCompute("lightspace: conetrace", glm::uvec3(res, 1));
        // disable glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);
      }

      // Blur the results
      if (true) {
        raytraceVoxelSpace_Blur
          ->use()
          ->uniformVec2ui("resolution", res)
          ->ssbo("outColorBuffer", raytraceOutput)
          ->ssbo("inTerminationBuffer", terminationOutput)
          ->ssbo("blueNoiseBuffer", blue_noise->ssbo)
          ->timedCompute("taa", glm::uvec3(res, 1));
      }
      // disable glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);
      // Debug rendering
      if (true) {
        glDisable(GL_DEPTH_TEST);
        debugBindless
          ->use()
          ->ssbo("inColorBuffer", raytraceOutput)
          ->ssbo("inTerminationBuffer", terminationOutput)
          ->uniformVec2ui("resolution", res);

        fullscreen_surface->render(debugBindless);
      }
    }

    {
      static float f = 0.0f;
      static int counter = 0;
      ImGui::Text("%.1fFPS (%.3f ms)", ImGui::GetIO().Framerate, 1000.0f / ImGui::GetIO().Framerate);
      ImGui::Text("camera(%.3f, %.3f, %.3f)", camera->Position.x, camera->Position.y, camera->Position.z);
      ImGui::Text("cat(%.3f, %.3f, %.3f)", catModel->getPosition().x, catModel->getPosition().y, catModel->getPosition().z);
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
