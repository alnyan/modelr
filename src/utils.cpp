#include "utils.h"

void *Utils::allocGlBuffer(GLuint &buffer, size_t sz, GLenum purpose, const void *data) {
    glGenBuffers(1, &buffer);
    glBindBuffer(purpose, buffer);
    glBufferStorage(purpose, sz, data, GL_DYNAMIC_STORAGE_BIT | GL_MAP_WRITE_BIT | GL_MAP_PERSISTENT_BIT | GL_MAP_COHERENT_BIT);
    return glMapBufferRange(purpose, 0, sz, GL_MAP_WRITE_BIT | GL_MAP_PERSISTENT_BIT | GL_MAP_COHERENT_BIT);
}

void *Utils::reallocGlBuffer(GLuint &buffer, size_t sz, GLenum purpose, const void *data) {
    glBindBuffer(purpose, buffer);
    glUnmapBuffer(purpose);
    glDeleteBuffers(1, &buffer);

    return allocGlBuffer(buffer, sz, purpose, data);
}

void *Utils::allocGlShaderSharedBuffer(GLuint &buffer, size_t sz, GLuint slot, const void *data) {
    glGenBuffers(1, &buffer);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, slot, buffer);
    glNamedBufferStorage(buffer, sz, data, GL_MAP_WRITE_BIT | GL_MAP_PERSISTENT_BIT | GL_MAP_COHERENT_BIT);
    return glMapNamedBufferRange(buffer, 0, sz, GL_MAP_WRITE_BIT | GL_MAP_PERSISTENT_BIT | GL_MAP_COHERENT_BIT);
}

void *Utils::reallocGlShaderSharedBuffer(GLuint &buffer, size_t sz, GLuint slot, const void *data) {
    glUnmapNamedBuffer(buffer);
    glDeleteBuffers(1, &buffer);

    return allocGlShaderSharedBuffer(buffer, sz, slot, data);
}
