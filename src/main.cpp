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
#include <string.h>
#include <queue>

bool keys[1024];
bool prevKeys[1024];
double mouse[2];
bool fullscreen = 0;
// int windowDimensions[2] = { 1024, 768 };
int windowDimensions[2] = { 1440, 900 };

glm::mat4 viewMatrix, perspectiveMatrix, MVP;
FBO *fbo = nullptr;

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
  if (fbo != nullptr) {
    fbo->setDimensions(width, height);
  }
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

  unsigned int time = 0;

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

  /*VZDParser::parse(
    "D:\\work\\voxviz\\include\\parser\\vzd\\out.vzd",
    volumeManager,
    physicsScene,
    bodyDef
  );*/

  /*
  VOXParser::parse(
    "D:\\work\\voxel-model\\vox\\character\\chr_cat.vox",
    volumeManager,
    physicsScene,
    bodyDef
  );

  Volume *tool = new Volume(glm::vec3(-5.0, 0 , 0.0));
  Brick *toolBrick = tool->AddBrick(glm::ivec3(1, 0, 0), &boxDef);
  toolBrick->createGPUMemory();
  toolBrick->fill(fillSphereProgram);
  
  

  Brick *toolBrick2 = tool->AddBrick(glm::ivec3(2, 0, 0), &boxDef);
  toolBrick2->createGPUMemory();
  toolBrick2->fillConst(0xFFFFFFFF);
  
  
  volumeManager->addVolume(tool);
  */
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
  
 // tool->scale.x = 1.0;
 // tool->scale.y = 1.0;
 // tool->scale.z = 1.0;
 // tool->rotation.z = M_PI / 4.0;
  //floor->rotation.z = M_PI / 2.0;

  volumeManager->addVolume(floor);

  //for (int x = 0; x < 32; x++) {
  //  for (int y = 0; y < 32; y++) {
  //    for (int z = 0; z < 32; z++) {
  //      floor->AddBrick(glm::ivec3(x, y, z));
  //    }
  //  }
  //}

  floor->AddBrick(glm::ivec3(0));

  //fillAllProgram->use()->uniform1ui("val", 0xFFFFFFFF);
  size_t i = 0;
  for (auto& it : floor->bricks) {
    i++;
    Brick *brick = it.second;
    brick->createGPUMemory();
    brick->fill(fillSphereProgram);
    //brick->fill(fillSphereProgram);
   // brick->fill(fillAllProgram);
  }


  bodyDef.bodyType = eDynamicBody;

  glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);

  FullscreenSurface *fullscreen_surface = new FullscreenSurface();
  fbo = new FBO(windowDimensions[0], windowDimensions[1]);
  
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
  const float movementSpeed = 0.01f;
  GLFWgamepadstate state;


  // Voxel space
  //const glm::uvec3 voxelSpaceDims = glm::uvec3(2000, 400, 3000);
  const glm::uvec3 voxelSpaceDims = glm::uvec3(512, 128, 512);
   
  uint64_t voxelSpaceBytes =
    static_cast<uint64_t>(voxelSpaceDims.x) *
    static_cast<uint64_t>(voxelSpaceDims.y) *
    static_cast<uint64_t>(voxelSpaceDims.z);
  SSBO *voxelSpaceSSBO = new SSBO(voxelSpaceBytes);

  SSBO *voxelSpaceSSBOMip1 = new SSBO(voxelSpaceBytes / 8);
  SSBO *voxelSpaceSSBOMip2 = new SSBO(voxelSpaceBytes / 16);
  SSBO *voxelSpaceSSBOMip3 = new SSBO(voxelSpaceBytes / 32);
  SSBO *voxelSpaceSSBOMip4 = new SSBO(voxelSpaceBytes / 64);
  SSBO *voxelSpaceSSBOMip5 = new SSBO(voxelSpaceBytes / 128);
  SSBO *voxelSpaceSSBOMip6 = new SSBO(voxelSpaceBytes / 128);

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
  
  Program *raytraceVoxelSpace_Blur = new Program();
  raytraceVoxelSpace_Blur->add(Shaders::get("voxel-space-blur.comp"))->link();

  Program *gravityVoxelSpace = new Program();
  gravityVoxelSpace->add(Shaders::get("voxel-space-gravity.comp"))->link();


  uint64_t outputBytes =
    static_cast<uint64_t>(windowDimensions[0]) *
    static_cast<uint64_t>(windowDimensions[1]) * 16;

  SSBO *raytraceOutput = new SSBO(outputBytes);
  uint64_t terminationBytes =
    static_cast<uint64_t>(windowDimensions[0]) *
    static_cast<uint64_t>(windowDimensions[1]) * 48;

  #define TAA_HISTORY_LENGTH 16
  SSBO *terminationOutput = new SSBO(terminationBytes * static_cast<uint64_t>(TAA_HISTORY_LENGTH));

  cout << "window dimensions: " << windowDimensions[0] << ", " << windowDimensions[1] << endl;
  // Draw a plane under the scene
  {
    fillVoxelSpace
      ->use()
      ->ssbo("volumeSlab", voxelSpaceSSBO, 1)
      ->uniform1ui("time", time)
      ->uniformVec3ui("dims", voxelSpaceDims);

    glDispatchCompute(
      voxelSpaceDims.x / 100,
      1,
      voxelSpaceDims.z / 1
    );
    gl_error();
    glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);
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

  // Load vox models
  VoxEntity *catModel = new VoxEntity(
    "D:\\work\\voxel-model\\vox\\character\\chr_cat.vox",
    glm::vec3(10.0, 10.0, 10.0)
  );

  {

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
    if (false) {
      // Clear the voxel space volume
      glm::uvec3 sdfDims(40, 40, 40);

      clearVoxelSpaceSDF
        ->use()
        ->ssbo("volumeSlab", voxelSpaceSSBO, 1)
        ->uniform1ui("time", lastCharacterTime)
        ->uniformVec3("offset", lastCharacterPos)
        ->uniformVec3ui("sdfDims", sdfDims)
        ->uniformVec3ui("dims", voxelSpaceDims)
        ->compute(sdfDims);

      // Fill the voxel space volume
      fillVoxelSpaceSDF
        ->use()
        ->ssbo("volumeSlab", voxelSpaceSSBO, 1)
        ->uniform1ui("time", time)
        ->uniformVec3("offset", characterPos)
        ->uniformVec3ui("sdfDims", sdfDims)
        ->uniformVec3ui("dims", voxelSpaceDims)
        ->compute(sdfDims);
      
      // Apply gravity to the scene
      if (time % 5 == 0) {
        gravityVoxelSpace
          ->use()
          ->ssbo("volumeSlab", voxelSpaceSSBO, 1)
          ->ssbo("blueNoise", blue_noise->ssbo, 2)
          ->uniform1ui("time", time)
          ->uniformVec3ui("dims", voxelSpaceDims)
          ->compute(glm::uvec3(
            voxelSpaceDims.x,
            1,
            voxelSpaceDims.z
          ));
      }
      lastCharacterTime = time;

      uint8_t *buf = (uint8_t *)voxelSpaceSSBO->beginMap(SSBO::MAP_WRITE_ONLY);
      catModel->paintInto(buf, voxelSpaceDims);
      voxelSpaceSSBO->endMap();

      gl_error();
      glMemoryBarrier(GL_ALL_BARRIER_BITS);
    }
    
    // Generate mipmaps
    if (true ||time % 5 == 0) {
      double mipStart = glfwGetTime();
      // Generate mipmap for SSBO (level 1)
      glm::uvec3 mipDims = voxelSpaceDims / glm::uvec3(2);
      mipmapVoxelSpace
        ->use()
        ->ssbo("sourceVolumeSlab", voxelSpaceSSBO, 1)
        ->ssbo("destVolumeSlab", voxelSpaceSSBOMip1, 2)
        ->uniformVec3ui("destDims", mipDims)
        ->compute(mipDims);
      gl_error();
    
      glMemoryBarrier(GL_ALL_BARRIER_BITS);

      // Generate mipmap for SSBO (level 2)
      mipDims = mipDims / glm::uvec3(2);
      mipmapVoxelSpace
        ->use()
        ->ssbo("sourceVolumeSlab", voxelSpaceSSBOMip1, 1)
        ->ssbo("destVolumeSlab", voxelSpaceSSBOMip2, 2)
        ->uniformVec3ui("destDims", mipDims)
        ->compute(mipDims);
      gl_error();
      glMemoryBarrier(GL_ALL_BARRIER_BITS);
      
      // Generate mipmap for SSBO (level 3)
      mipDims = mipDims / glm::uvec3(2);
      mipmapVoxelSpace
        ->use()
        ->ssbo("sourceVolumeSlab", voxelSpaceSSBOMip2, 1)
        ->ssbo("destVolumeSlab", voxelSpaceSSBOMip3, 2)
        ->uniformVec3ui("destDims", mipDims)
        ->compute(mipDims);
      gl_error();
      glMemoryBarrier(GL_ALL_BARRIER_BITS);
       
      // Generate mipmap for SSBO (level 4)
      mipDims = mipDims / glm::uvec3(2);
      mipmapVoxelSpace
        ->use()
        ->ssbo("sourceVolumeSlab", voxelSpaceSSBOMip3, 1)
        ->ssbo("destVolumeSlab", voxelSpaceSSBOMip4, 2)
        ->uniformVec3ui("destDims", mipDims)
        ->compute(mipDims);
      gl_error();
      glMemoryBarrier(GL_ALL_BARRIER_BITS);

      // Generate mipmap for SSBO (level 5)
      mipDims = mipDims / glm::uvec3(2);
      mipmapVoxelSpace
        ->use()
        ->ssbo("sourceVolumeSlab", voxelSpaceSSBOMip4, 1)
        ->ssbo("destVolumeSlab", voxelSpaceSSBOMip5, 2)
        ->uniformVec3ui("destDims", mipDims)
        ->compute(mipDims);
      gl_error();
      glMemoryBarrier(GL_ALL_BARRIER_BITS);

      // Generate mipmap for SSBO (level 6)
      mipDims = mipDims / glm::uvec3(2);
      mipmapVoxelSpace
        ->use()
        ->ssbo("sourceVolumeSlab", voxelSpaceSSBOMip5, 1)
        ->ssbo("destVolumeSlab", voxelSpaceSSBOMip6, 2)
        ->uniformVec3ui("destDims", mipDims)
        ->compute(mipDims);
      gl_error();
      glMemoryBarrier(GL_ALL_BARRIER_BITS);

      ImGui::Text("mipmaps: %f", (glfwGetTime() - mipStart) * 1000.0);
    }
    
    // Render the voxel space volume
    if (false) {
      raytraceVoxelSpace
        ->use()
        ->uniformVec3("eye", currentEye)
        ->ssbo("volumeSlabMip0", voxelSpaceSSBO, 1)
        ->ssbo("volumeSlabMip1", voxelSpaceSSBOMip1, 2)
        ->ssbo("volumeSlabMip2", voxelSpaceSSBOMip2, 3)
        ->ssbo("volumeSlabMip3", voxelSpaceSSBOMip3, 4)
        ->uniformFloat("maxDistance", 10000)
        ->uniformMat4("VP", VP)
        ->uniform1ui("time", time)
        ->uniformVec3ui("lightPos", glm::uvec3(
          10 + static_cast<uint32_t>(abs(sin(nowTime / 10.0) * 200)),
          20,
          0
        ))
        ->uniformVec3("lightColor", glm::vec3(1.0, 1.0, 1.0))
        ->uniformVec3("dims", glm::vec3(voxelSpaceDims));

      fullscreen_surface->render(raytraceVoxelSpace);
    }

    // Raytrace in compute
    {
      // Raytrace into `raytraceOutput`
      glm::uvec2 res = glm::uvec2(windowDimensions[0], windowDimensions[1]);
      {
        raytraceVoxelSpace_Compute
          ->use()

          ->uniformVec3("eye", currentEye)
          ->ssbo("volumeSlabMip0", voxelSpaceSSBO, 1)
          ->ssbo("volumeSlabMip1", voxelSpaceSSBOMip1, 2)
          ->ssbo("volumeSlabMip2", voxelSpaceSSBOMip2, 3)
          ->ssbo("volumeSlabMip3", voxelSpaceSSBOMip3, 4)
          ->ssbo("volumeSlabMip4", voxelSpaceSSBOMip4, 5)
          ->ssbo("volumeSlabMip5", voxelSpaceSSBOMip5, 6)
          ->ssbo("volumeSlabMip6", voxelSpaceSSBOMip6, 7)
          ->ssbo("outColorBuffer", raytraceOutput, 8)
          ->ssbo("outTerminationBuffer", terminationOutput, 9)
          ->ssbo("blueNoiseBuffer", blue_noise->ssbo, 10)

          ->uniformFloat("maxDistance", 10000)
          ->uniformMat4("VP", VP)
          ->uniform1ui("time", time)
          ->uniformFloat("debug", debug)
          ->uniformVec3ui("lightPos", glm::uvec3(
            10 + static_cast<uint32_t>(abs(sin(nowTime / 10.0) * 200)),
            20,
            0
          ))
          ->uniformVec3("lightColor", glm::vec3(1.0, 1.0, 1.0))
          ->uniformVec3("dims", glm::vec3(voxelSpaceDims))

          ->uniformVec3("characterPos", catModel->getPosition())

          ->uniformVec2ui("resolution", res)
          ->uniform1ui("terminationBufferIdx", 0);//res.x * res.y * (time%TAA_HISTORY_LENGTH));

        glDispatchCompute(
          windowDimensions[0] / 128 + 1,
          windowDimensions[1] / 8 + 1,
          1
        );
      }
      glMemoryBarrier(GL_ALL_BARRIER_BITS);
      
      
      // Blur the results
      {
        raytraceVoxelSpace_Blur
          ->use()
          ->uniformVec2ui("resolution", glm::uvec2(windowDimensions[0], windowDimensions[1]))
          ->uniform1ui("time", time)
          ->uniformVec3("eye", currentEye)
          ->uniformVec3("dims", glm::vec3(voxelSpaceDims))
          ->uniformFloat("debug", debug)
          ->ssbo("outColorBuffer", raytraceOutput, 1)
          ->ssbo("inTerminationBuffer", terminationOutput, 2)
          ->ssbo("blueNoiseBuffer", blue_noise->ssbo, 7)
          ->uniform1ui("terminationBufferIdx", res.x * res.y * (time%TAA_HISTORY_LENGTH));

        glDispatchCompute(
          windowDimensions[0] / 128 + 1,
          windowDimensions[1] / 8 + 1,
          1
        );
      }
      glMemoryBarrier(GL_ALL_BARRIER_BITS);
      // Debug rendering
      {
        glDisable(GL_DEPTH_TEST);
        debugBindless
          ->use()
          ->ssbo("inColorBuffer", raytraceOutput, 1)
          ->ssbo("inTerminationBuffer", terminationOutput, 2)
          ->uniformVec2ui("resolution", glm::uvec2(windowDimensions[0], windowDimensions[1]));

          

        fullscreen_surface->render(debugBindless);
      }
    }
    
    {
      static float f = 0.0f;
      static int counter = 0;
      ImGui::Text("%.3f ms/frame (%.1f FPS)", 1000.0f / ImGui::GetIO().Framerate, ImGui::GetIO().Framerate);
      ImGui::Text("camera pos(%.3f, %.3f, %.3f)", camera->Position.x, camera->Position.y, camera->Position.z);
      ImGui::Text("%i floor bricks", floor->bricks.size());
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
  uv_read_stop((uv_stream_t*)&stdin_pipe);
  uv_stop(uv_default_loop());
  uv_loop_close(uv_default_loop());
  glfwTerminate();
  return 0;
}
