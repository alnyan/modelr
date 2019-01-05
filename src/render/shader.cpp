#include "shader.h"
#include "../res/assets.h"
#include <stdarg.h>
#include <iostream>
#include <fstream>
#include <string>
#include <list>

bool Shader::loadShader(GLenum type, const char *path, GLuint &shaderID) {
    std::cout << "Load shader " << path << std::endl;

    std::string shaderCode;
    std::ifstream file(Assets::getShaderPath(path));
    const char *shaderCodePtr;
    GLint status, errorLength;

    if (!file) {
        return false;
    }

    shaderCode = std::string(std::istreambuf_iterator<char>(file), std::istreambuf_iterator<char>());
    shaderID = glCreateShader(type);

    if (!shaderID) {
        return false;
    }

    shaderCodePtr = shaderCode.c_str();
    glShaderSource(shaderID, 1, &shaderCodePtr, NULL);
    glCompileShader(shaderID);

    glGetShaderiv(shaderID, GL_COMPILE_STATUS, &status);
    if (!status) {
        glGetShaderiv(shaderID, GL_INFO_LOG_LENGTH, &errorLength);
        char *error = new char[errorLength + 1];
        error[errorLength] = 0;
        glGetShaderInfoLog(shaderID, errorLength, NULL, error);
        std::cerr << "Shader compilation error: " << path << std::endl << error << std::endl;
        delete error;
        glDeleteShader(shaderID);

        return false;
    }

    return true;
}

bool Shader::loadProgram(GLuint &program, int count, ...) {
    std::cout << "Load program" << std::endl;

    std::list<GLuint> shaders;
    va_list args;
    GLint status, errorLength;

    // Create shader program
    program = glCreateProgram();

    if (!program) {
        return false;
    }

    // Read all shaders
    va_start(args, count);
    for (int i = 0; i < count; ++i) {
        GLuint shader;
        GLenum type = va_arg(args, GLenum);
        const char *path = va_arg(args, const char *);

        if (!loadShader(type, path, shader)) {
            // Delete previous shaders
            for (GLuint id: shaders) {
                glDeleteShader(id);
            }

            return false;
        }

        shaders.push_back(shader);
    }
    va_end(args);

    // Attach shaders to program
    for (GLuint id: shaders) {
        glAttachShader(program, id);
    }

    // Link program
    glLinkProgram(program);

    // Detach/delete shaders
    for (GLuint id: shaders) {
        glDetachShader(program, id);
        glDeleteShader(id);
    }

    // Get link status
    glGetProgramiv(program, GL_LINK_STATUS, &status);

    if (!status) {
        glGetProgramiv(program, GL_INFO_LOG_LENGTH, &errorLength);
        char *error = new char[errorLength + 1];
        error[errorLength] = 0;
        glGetProgramInfoLog(program, errorLength, NULL, error);
        std::cerr << "Program linking error:" << std::endl << error << std::endl;
        delete error;
        glDeleteProgram(program);
        return false;
    }

    return true;
}
