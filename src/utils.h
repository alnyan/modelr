#pragma once
#include <GL/glew.h>

namespace Utils {

    void *allocGlBuffer(GLuint &buffer, size_t sz, GLenum purpose, const void *data = NULL);
    void *reallocGlBuffer(GLuint &buffer, size_t sz, GLenum purpose, const void *data);
    void *allocGlShaderSharedBuffer(GLuint &buffer, size_t sz, GLuint loc, const void *data = NULL);
    void *reallocGlShaderSharedBuffer(GLuint &buffer, size_t sz, GLuint loc, const void *data);

}
