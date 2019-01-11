#include "shader.h"
#include "../res/assets.h"
#include <stdarg.h>
#include <string.h>
#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <list>
#include <map>

static std::map<std::string, std::string> s_shaderDefinitions {
    { "SHADER_PROCESSED", "1" }
};

void Shader::addDefinition(const std::string &name, const std::string &value) {
    s_shaderDefinitions[name] = value;
}

bool Shader::loadShader(GLenum type, const char *path, GLuint &shaderID) {
    std::cout << "Load shader " << path << std::endl;

    std::string shaderCode;
    std::stringstream shaderBuilder;
    std::ifstream file(Assets::getShaderPath(path));
    const char *shaderCodePtr;
    GLint status, errorLength;

    if (!file) {
        return false;
    }

    shaderID = glCreateShader(type);

    if (!shaderID) {
        return false;
    }

    std::string line;
    bool dirsInserted = false;
    while (std::getline(file, line)) {
        shaderBuilder << line << "\n";
        if (!dirsInserted &&
            !line.empty() &&
            !strncmp(line.c_str(), "#version ", 9)) {
            for (const auto &[name, value]: s_shaderDefinitions) {
                shaderBuilder << "#define " << name << " " << value << "\n";
            }
            dirsInserted = true;
        }
    }

    shaderCode = shaderBuilder.str();

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
