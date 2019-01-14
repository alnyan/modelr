#pragma once
#include <GL/glew.h>
#include <glm/glm.hpp>
#include "utils.h"

template<typename T, GLenum P, GLuint B = 0xFF> struct GlDynamicArray {
    T *data = nullptr;
    GLuint bufferID = 0;
    GLuint capacity = 32;
    GLuint size = 0;

    void generate() {
        if constexpr (P == GL_SHADER_STORAGE_BUFFER) {
            data = (T *) Utils::allocGlShaderSharedBuffer(bufferID, sizeof(T) * capacity, B);
        } else {
            data = (T *) Utils::allocGlBuffer(bufferID, sizeof(T) * capacity, P);
        }

        if (!data) {
            throw std::runtime_error("Failed to reallocate buffers");
        }
    }

    void expand(GLuint newCapacity) {
        std::cout << "realloc: " << capacity << " -> " << newCapacity << std::endl;
        // Realloc buffer
        capacity = newCapacity;
        if constexpr (P == GL_SHADER_STORAGE_BUFFER) {
            data = (T *) Utils::reallocGlShaderSharedBuffer(bufferID, sizeof(T) * capacity, B, data);
        } else {
            data = (T *) Utils::reallocGlBuffer(bufferID, sizeof(T) * capacity, P, data);
        }
        if (!data) {
            throw std::runtime_error("Failed to reallocate buffers");
        }
    }

    void remove(size_t idx) {
        assert(idx < size);
        assert(data);

        for (size_t i = size - 1; i > idx; --i) {
            data[i - 1] = data[i];
        }
        --size;

        if (capacity / 2 > size + capacity / 4) {
            expand(capacity / 2);
        }
    }

    void append(const T &obj) {
        assert(data);

        if (size + 1 >= capacity) {
            expand(capacity * 2);
        }

        data[size] = obj;
        ++size;
    }

    const T &operator [](size_t i) const {
        return data[i];
    }

    T &operator [](size_t i) {
        return data[i];
    }
};
