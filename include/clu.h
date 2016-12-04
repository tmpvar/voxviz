#ifndef __CLU_H__
#define __CLU_H__

  #if defined(__APPLE__)
    #include <OpenCL/opencl.h>
    #include <GLFW/glfw3.h>
    #include <OpenGL/OpenGL.h>
    #include <OpenCL/opencl.h>
    #include <OpenCL/cl_gl.h>
  #elif defined(_WIN32)
    #include <CL/cl.h>
    #include <CL/cl_gl.h>
    #include <Windows.h>
  #endif

  #include <stdio.h>
  #include <iostream>
  using namespace std;

  #define MEM_SIZE (128)
  #define MAX_SOURCE_SIZE (0x100000)
  #define CL_CHECK_ERROR(err) do{if (err) {printf("FATAL ERROR %d at " __FILE__ ":%d\n",err,__LINE__); clu_error(err); exit(1); } } while(0)

  void clu_error(cl_int err);

  void clu_print_program_info(cl_device_id device, cl_program program);

  void clu_print_device_info_string(cl_device_id device, cl_int type, const char *fmt);
  void clu_print_device_extensions(cl_device_id device);
  void clu_print_max_allocation_size(cl_device_id d);
  void clu_print_device_info(cl_device_id d);
  cl_int clu_program_from_string(cl_device_id device, cl_context context, const char* source_str, size_t source_size, cl_program *program);
  cl_int clu_program_from_fs(cl_device_id device, cl_context context, const char *file, cl_program *program);

  namespace cl {
    template <class T> class Buffer {
      public:
      cl_mem mem;
      cl_context context;
      size_t length;
      size_t element_size;
      size_t byte_length;

      Buffer<T>(cl_context context, size_t length) {
        this->init(context, CL_MEM_READ_WRITE, length);
      }

      Buffer<T>(cl_context context, cl_mem_flags flags, size_t length) {
        this->init(context, flags, length);
      }

      Buffer<T>(cl_context context, cl_mem_flags flags, size_t length, T *data) {
        this->init(context, flags | CL_MEM_COPY_HOST_PTR, length, data);
      }

      Buffer<T>(cl_context context, size_t length, T *data) {
        this->init(context, CL_MEM_READ_WRITE | CL_MEM_COPY_HOST_PTR, length, data);
      }

      void init(cl_context context, cl_mem_flags flags, size_t length, T *data = NULL) {
        this->context = context;
        this->length = length;
        this->element_size = sizeof(T);
        this->byte_length = this->element_size * length;

        cl_int ret;
        this->mem = clCreateBuffer(
          this->context,
          flags,
          this->byte_length,
          data,
          &ret
        );

        if (ret) {
          printf("failed to allocate %lu bytes\n", this->byte_length);
        }

        CL_CHECK_ERROR(ret);
      }

      void print(cl_command_queue queue) {

        T *buf = (T *)clEnqueueMapBuffer(
          queue,
          this->mem,
          CL_TRUE,
          CL_MAP_READ,
          0,
          this->byte_length,
          0,
          NULL,
          NULL,
          NULL
        );

        for (size_t i = 0; i < this->length; i++) {
          cout << buf[i] << " ";
        }

        cout << endl;
        cout.flush();

        cl_int ret;

        ret = clEnqueueUnmapMemObject(queue, this->mem, buf, 0, NULL, NULL);
        clFinish(queue);
      }

      void bind(cl_kernel kernel, cl_uint index) {
        CL_CHECK_ERROR(
          clSetKernelArg(kernel, index, sizeof(cl_mem), &this->mem)
        );
      }
    };

    // TODO: print/slice/etc

    typedef Buffer<cl_uint> UIntArray;
    typedef Buffer<cl_int> IntArray;
    typedef Buffer<cl_float> FloatArray;
  }

  typedef struct {
    cl_device_id device;
    cl_context context;
    cl_command_queue command_queue;
    cl_program program;
    cl_kernel kernel;
  } clu_job_t;

  int clu_compute_init(clu_job_t *job);
  void clu_compute_destroy(clu_job_t *job);
#endif
