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
#include "voxel-cascade.h"
#include "uniform-grid.h"

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
#include <set>

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

struct DepthBucket {
  Brick **bricks;
  size_t max_size;
  size_t pos;
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
    //glm::vec3(0, 0, 0)
  );

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

  // VOXEL CASCADE
  VoxelCascade *voxelCascade = new VoxelCascade(TOTAL_VOXEL_CASCADE_LEVELS);

  // UniformGrid
  //UniformGrid *uniformGrid = new UniformGrid(glm::uvec3(256), 32);
  UniformGrid *uniformGrid = new UniformGrid(glm::uvec3(128),2);

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
    
    int max_block_size;
    glGetIntegerv(GL_MAX_SHADER_STORAGE_BLOCK_SIZE, &max_block_size);
    printf("GL_MAX_SHADER_STORAGE_BLOCK_SIZE %i\n", max_block_size);    

    int max_blocks;
    glGetIntegerv(GL_MAX_FRAGMENT_SHADER_STORAGE_BLOCKS, &max_blocks);
    printf("GL_MAX_FRAGMENT_SHADER_STORAGE_BLOCKS %i\n", max_blocks);
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
  */

  Volume *tool = new Volume(glm::vec3(0.0, 0, 0.0));
  Brick *toolBrick = tool->AddBrick(glm::ivec3(-2, 0, 0), &boxDef);
  toolBrick->createGPUMemory();
  toolBrick->fill(fillSphereProgram);
  
  //tool->scale.x = 10.0;
  //tool->scale.y = 10.0;
  //tool->scale.z = 10.0;

  

  Brick *toolBrick2 = tool->AddBrick(glm::ivec3(-4, 0, 0), &boxDef);
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
  
 // tool->rotation.z = M_PI / 4.0;
  //floor->rotation.z = M_PI / 2.0;

  volumeManager->addVolume(floor);
  int floor_spacing = 2;
  for (int x = 0; x < 16; x+=floor_spacing) {
    for (int y = 0; y < 16; y+=floor_spacing) {
      for (int z = 0; z < 64; z+=floor_spacing) {
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
  
  // Depth buckets
  #define DEPTH_BUCKET_SIZE 1 
  #define MAX_DEPTH_BUCKETS MAX_DISTANCE/DEPTH_BUCKET_SIZE
  
  DepthBucket *depthBuckets = (DepthBucket *)malloc(sizeof(DepthBucket) * MAX_DEPTH_BUCKETS);
  
  for (size_t i = 0; i < MAX_DEPTH_BUCKETS; i++) {
    depthBuckets[i].pos = 0;
    depthBuckets[i].max_size = 1024;
    depthBuckets[i].bricks = (Brick **)malloc(sizeof(Brick *) * depthBuckets[i].max_size);
  }

  double time = glfwGetTime();
  while (!glfwWindowShouldClose(window)) {
    double now = glfwGetTime();
    double deltaTime = now - time;
    time = now;

    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();

    float speed = movementSpeed * (keys[GLFW_KEY_LEFT_SHIFT] ? 100.0 : 10.0);
    if (keys[GLFW_KEY_W]) {
      camera->ProcessKeyboard(Camera_Movement::FORWARD, speed * deltaTime);
      //camera->translate(0.0f, 0.0f, 10.0f);
    }

    if (keys[GLFW_KEY_S]) {
      camera->ProcessKeyboard(Camera_Movement::BACKWARD, speed * deltaTime);
    }

    if (keys[GLFW_KEY_A]) {
      camera->ProcessKeyboard(Camera_Movement::LEFT, speed * deltaTime);
    }

    if (keys[GLFW_KEY_D]) {
      camera->ProcessKeyboard(Camera_Movement::RIGHT, speed * deltaTime);
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

    for (int i = 0; i < 512; i++) {
      if (volumeManager->tick()) {
        break;
      }
    }
    
    // Depth bucketting
    if (false) {
      double start = glfwGetTime();
      ImGui::Begin("depth buckets");
      ImVec2 pos = { 0, 200 };
      ImVec2 dims = { 300, 700 };
      ImGui::SetWindowPos(pos);
      ImGui::SetWindowSize(dims);

      for (size_t i = 0; i < MAX_DEPTH_BUCKETS; i++) {
        if (depthBuckets[i].pos > depthBuckets[i].max_size) {
          ImGui::Text("malloc bucket %i", i);
          depthBuckets[i].max_size = (size_t)(ceilf(depthBuckets[i].pos / 1024.0f) * 1024);
          free(depthBuckets[i].bricks);
          depthBuckets[i].bricks = (Brick **)malloc(sizeof(Brick *) * depthBuckets[i].max_size);
        }

        depthBuckets[i].pos = 0;
      }

      for (auto& volume : volumeManager->volumes) {
        Brick *brick;
        glm::mat4 mat = volume->getModelMatrix();
        glm::vec3 invEye = glm::inverse(mat) * glm::vec4(currentEye, 1.0);

        for (auto& it : volume->bricks) {
          brick = it.second;
          
          float dx = invEye.x - (brick->index.x + 0.5f);
          float dy = invEye.y - (brick->index.y + 0.5f);
          float dz = invEye.z - (brick->index.z + 0.5f);

          float distance = sqrtf(dx*dx + dy*dy + dz*dz);

          size_t bucket = (size_t)(floorf(logf(distance / 10)));
          if (depthBuckets[bucket].pos < depthBuckets[bucket].max_size) {
            depthBuckets[bucket].bricks[depthBuckets[bucket].pos] = brick;
          }
          depthBuckets[bucket].pos++;
        }
      }

      for (size_t i = 0; i < MAX_DEPTH_BUCKETS; i++) {
        if (depthBuckets[i].pos > 0) {
          ImGui::Text("depth bucket %i entries %i (%i)", i, depthBuckets[i].pos, depthBuckets[i].max_size);
        }
      }
      ImGui::End();
    }



    //Normal Render Everything approach
    if (keys[GLFW_KEY_TAB]) {
      for (auto& volume : volumeManager->volumes) {
        if (volume->bricks.size() == 0) {
          continue;
        }

        glm::mat4 volumeModel = volume->getModelMatrix();
        raytracer->program->use()
          ->uniformMat4("MVP", VP * volumeModel)
          ->uniformMat4("model", volumeModel)
          ->uniformVec3("eye", currentEye)
          ->uniformFloat("maxDistance", max_distance)
          ->uniform1i("showHeat", raytracer->showHeat)
          ->uniformVec4("material", volume->material);

        gl_error();
        size_t activeBricks = volume->bind();
        //raytracer->render(volume, raytracer->program);

        glDrawElementsInstanced(
          GL_TRIANGLES,
          volume->mesh->faces.size(),
          GL_UNSIGNED_INT,
          0,
          //volume->bricks.size()
          activeBricks
        );
        gl_error();
      }
     }
    
    //if (keys[GLFW_KEY_TAB]) {
      // Uniform Grid

      ImGui::Begin("stats");
      double start = glfwGetTime();
      if (uniformGrid->begin(camera->Position)) {
        for (auto& volume : volumeManager->volumes) {
          uniformGrid->addVolume(volume);
        }
        
        uniformGrid->end();
        
      }
      else {

      }
      uniformGrid->debugRaytrace(
        perspectiveMatrix * viewMatrix,
        currentEye
      );

      ImGui::Text("uniformgrid time: %.3fms", (glfwGetTime() - start) * 1000);
      ImGui::End();

      /*
      // Voxel Cascade
      ImGui::Begin("stats");
      double start = glfwGetTime();
      voxelCascade->begin(camera->Position);

      for (auto& volume : volumeManager->volumes) {
        voxelCascade->addVolume(volume);
      }
      voxelCascade->end();

      voxelCascade->debugRender(VP);
      voxelCascade->debugRaytrace(perspectiveMatrix * viewMatrix);
      
      ImGui::Text("cascade time: %.3fms", (glfwGetTime() - start) * 1000);
      ImSGui::End();
      */
   // } else {



    //}    

    //volumeManager->volumes[0]->rotation.x += 0.0001;
    //volumeManager->volumes[1]->scale.z = 1.0 + abs(sin(time / 1000.0)) * 10.0;
    //volumeManager->volumes[1]->rotation.y += 0.001;
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
    //floor->rotation.z += 0.001;
    //tool->rotation.z += 0.001;
    //tool->rotation.y += 0.002;
    tool->rotation.z += deltaTime * 1.0;
    tool->scale.x = 1.0 + fabs(sinf(float(time) / 10.0) * 20.0);
    tool->scale.y = 1.0 + fabs(sinf(float(time) / 5.0) * 20.0);
    tool->scale.z = 1.0 + fabs(sinf(float(time) / 2.0) * 20.0);
    
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
    

    {
      ImGui::Begin("stats");
      ImVec2 size = { 400, 400};
      ImGui::SetWindowSize(size, 0);
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
    //time++;
    
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
