#include "gl-wrap.h"

GLint gl_ok(GLint error) {
  switch (error) {
  case GL_INVALID_ENUM:
    printf("error (%i): GL_INVALID_ENUM: An unacceptable value is specified for an enumerated argument. The offending command is ignored and has no other side effect than to set the error flag.\n", error);
    break;

  case GL_INVALID_VALUE:
    printf("error (%i): GL_INVALID_VALUE: A numeric argument is out of range. The offending command is ignored and has no other side effect than to set the error flag.\n", error);
    break;

  case GL_INVALID_OPERATION:
    printf("error (%i): GL_INVALID_OPERATION: The specified operation is not allowed in the current state. The offending command is ignored and has no other side effect than to set the error flag.\n", error);
    break;
#ifdef GL_STACK_OVERFLOW
  case GL_STACK_OVERFLOW:
    printf("error (%i): GL_STACK_OVERFLOW: This command would cause a stack overflow. The offending command is ignored and has no other side effect than to set the error flag.\n", error);
    break;
#endif

#ifdef GL_STACK_UNDERFLOW
  case GL_STACK_UNDERFLOW:
    printf("error (%i): GL_STACK_UNDERFLOW: This command would cause a stack underflow. The offending command is ignored and has no other side effect than to set the error flag.\n", error);
    break;
#endif

  case GL_OUT_OF_MEMORY:
    printf("error (%i): GL_OUT_OF_MEMORY: There is not enough memory left to execute the command. The state of the GL is undefined, except for the state of the error flags, after this error is recorded.\n", error);
    break;

  case GL_FRAMEBUFFER_UNDEFINED: 
    printf("error (%i): GL_FRAMEBUFFER_UNDEFINED is returned if the specified framebuffer is the default read or draw framebuffer, but the default framebuffer does not exist.", error);
    break;
    
  case GL_FRAMEBUFFER_INCOMPLETE_ATTACHMENT: 
    printf("error (%i): GL_FRAMEBUFFER_INCOMPLETE_ATTACHMENT is returned if any of the framebuffer attachment points are framebuffer incomplete.", error);
    break;
    
  case GL_FRAMEBUFFER_INCOMPLETE_MISSING_ATTACHMENT: 
    printf("error (%i): GL_FRAMEBUFFER_INCOMPLETE_MISSING_ATTACHMENT is returned if the framebuffer does not have at least one image attached to it.", error);
    break;
    
  case GL_FRAMEBUFFER_INCOMPLETE_DRAW_BUFFER: 
    printf("error (%i): GL_FRAMEBUFFER_INCOMPLETE_DRAW_BUFFER is returned if the value of GL_FRAMEBUFFER_ATTACHMENT_OBJECT_TYPE is GL_NONE for any color attachment point(s) named by GL_DRAW_BUFFERi.", error);
    break;
    
  case GL_FRAMEBUFFER_INCOMPLETE_READ_BUFFER: 
    printf("error (%i): GL_FRAMEBUFFER_INCOMPLETE_READ_BUFFER is returned if GL_READ_BUFFER is not GL_NONE and the value of GL_FRAMEBUFFER_ATTACHMENT_OBJECT_TYPE is GL_NONE for the color attachment point named by GL_READ_BUFFER.", error);
    break;
    
  case GL_FRAMEBUFFER_UNSUPPORTED: 
    printf("error (%i): GL_FRAMEBUFFER_UNSUPPORTED is returned if the combination of internal formats of the attached images violates an implementation - dependent set of restrictions.", error);
    break;
    
  case GL_FRAMEBUFFER_INCOMPLETE_MULTISAMPLE: 
    printf("error (%i): GL_FRAMEBUFFER_INCOMPLETE_MULTISAMPLE is returned if the value of GL_RENDERBUFFER_SAMPLES is not the same for all attached renderbuffers; if the value of GL_TEXTURE_SAMPLES is the not same for all attached textures; or , if the attached images are a mix of renderbuffers and textures, the value of GL_RENDERBUFFER_SAMPLES does not match the value of GL_TEXTURE_SAMPLES.", error);
    printf("error (%i): GL_FRAMEBUFFER_INCOMPLETE_MULTISAMPLE is also returned if the value of GL_TEXTURE_FIXED_SAMPLE_LOCATIONS is not the same for all attached textures; or , if the attached images are a mix of renderbuffers and textures, the value of GL_TEXTURE_FIXED_SAMPLE_LOCATIONS is not GL_TRUE for all attached textures.", error);
    break;
    
  case GL_FRAMEBUFFER_INCOMPLETE_LAYER_TARGETS: 
    printf("error (%i): GL_FRAMEBUFFER_INCOMPLETE_LAYER_TARGETS is returned if any framebuffer attachment is layered, and any populated attachment is not layered, or if all populated color attachments are not from textures of the same target.", error);
    break;
  }
  return error;
}

GLint GL_ERROR() {
  GLint ret = 0;
  GLint err = 0;
  do {
    err = gl_ok(glGetError());
    if (err) {
      ret = err;
    }

  } while (err);
 

  return ret;
}

void gl_shader_log(GLuint shader) {
  GLint l, m;
  GLint isCompiled = 0;
  glGetShaderiv(shader, GL_COMPILE_STATUS, &isCompiled);
  if (isCompiled == GL_FALSE) {
    glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &m);
    char *s = (char *)malloc(m * sizeof(char));
    glGetShaderInfoLog(shader, m, &l, s);
    printf("shader log:\n%s\n", s);
    free(s);
  }
}


void gl_program_log(GLuint handle) {
  GLint error = glGetError();
  GLint l, m;
  GLint isLinked = 0;
  
  glGetProgramiv(handle, GL_LINK_STATUS, &isLinked);
  if (isLinked == GL_FALSE) {
    glGetShaderiv(handle, GL_INFO_LOG_LENGTH, &m);
    char *s = (char *)malloc(m * sizeof(char));
    glGetProgramInfoLog(handle, m, &l, s);
    printf("program log:\n%s\n", s);
    free(s);
  }
}