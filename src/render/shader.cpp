#include "shader.h"
#include <iostream>
#include <fstream>
#include <stdlib.h>
#include "material.h"
#include "../res/assets.h"

Shader::Shader(GLuint programID): m_programID{programID} {
    glGenBuffers(1, &m_materialBufferID);

    glBindBuffer(GL_UNIFORM_BUFFER, m_materialBufferID);
    glBufferData(GL_UNIFORM_BUFFER, 16 * 4, NULL, GL_DYNAMIC_DRAW);
    glBindBuffer(GL_UNIFORM_BUFFER, 0);

    glBindBufferBase(GL_UNIFORM_BUFFER, 1, m_materialBufferID);

    // Not changed
}

Shader::~Shader() {
    glDeleteProgram(m_programID);
    glDeleteBuffers(1, &m_materialBufferID);
}

void Shader::apply() {
    glUseProgram(m_programID);
}

void Shader::applyMaterial(Material *mat) {
    glNamedBufferSubData(m_materialBufferID, 0, sizeof(MaterialUniformObject), &mat->uniformData);
}

static bool loadShaderSingle(const std::string &filename, GLenum shaderType, GLuint &shaderID) {
    std::string shaderCode;
    std::ifstream file(Assets::getShaderPath(filename));
    GLint errorLength;
    GLint status;

    if (!file) {
        std::cerr << "File does not exist: " << Assets::getShaderPath(filename) << std::endl;
        return false;
    }

    shaderCode = std::string(std::istreambuf_iterator<char>(file), std::istreambuf_iterator<char>());
    const char *shaderCodep = shaderCode.c_str();
    shaderID = glCreateShader(shaderType);
    glShaderSource(shaderID, 1, &shaderCodep, nullptr);
    glCompileShader(shaderID);

    glGetShaderiv(shaderID, GL_COMPILE_STATUS, &status);

    if (status != GL_TRUE) {
        // Compilation failed
        glGetShaderiv(shaderID, GL_INFO_LOG_LENGTH, &errorLength);
        char *error = (char *) malloc(errorLength + 1);
        error[errorLength] = 0;
        glGetShaderInfoLog(shaderID, errorLength, nullptr, error);

        std::cerr << "Failed to compile shader: " << filename << std::endl << error << std::endl;

        glDeleteShader(shaderID);
        return false;
    }

    return true;
}

Shader *Shader::loadShader(const std::string &vertFile, const std::string &fragFile) {
    GLuint vertShaderID, fragShaderID;
    GLint status, errorLength;

    if (!loadShaderSingle(vertFile, GL_VERTEX_SHADER, vertShaderID)) {
        std::cerr << "Failed to load vertex shader" << std::endl;
        return nullptr;
    }

    if (!loadShaderSingle(fragFile, GL_FRAGMENT_SHADER, fragShaderID)) {
        std::cerr << "Failed to load fragment shader" << std::endl;
        glDeleteShader(vertShaderID);
        return nullptr;
    }

    GLuint progID = glCreateProgram();
    glAttachShader(progID, vertShaderID);
    glAttachShader(progID, fragShaderID);
    glLinkProgram(progID);

    glGetProgramiv(progID, GL_LINK_STATUS, &status);
    if (status != GL_TRUE) {
        glGetProgramiv(progID, GL_INFO_LOG_LENGTH, &errorLength);
        char *error = (char *) malloc(errorLength + 1);
        error[errorLength] = 0;
        glGetProgramInfoLog(progID, errorLength, nullptr, error);

        std::cerr << "Failed to link shader program:" << std::endl << error << std::endl;

        glDetachShader(progID, vertShaderID);
        glDetachShader(progID, fragShaderID);
        glDeleteProgram(progID);
        glDeleteShader(vertShaderID);
        glDeleteShader(fragShaderID);

        return nullptr;
    }

    glDetachShader(progID, vertShaderID);
    glDetachShader(progID, fragShaderID);
    glDeleteShader(vertShaderID);
    glDeleteShader(fragShaderID);

    return new Shader(progID);
}

GLuint Shader::getUniformLocation(const std::string &name) {
    return glGetUniformLocation(m_programID, name.c_str());
}
