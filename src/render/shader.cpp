#include "shader.h"
#include <iostream>
#include <fstream>
#include <stdlib.h>

Shader::Shader(GLuint programID): m_programID{programID} {
    if (m_type == SH_MATERIAL) {
        MaterialShaderData *sh = new MaterialShaderData;
        m_shaderData = sh;

        sh->m_map_Kd = glGetUniformLocation(m_programID, "m_map_Kd");
        sh->m_map_Bump = glGetUniformLocation(m_programID, "m_map_Bump");
        sh->m_Kd = glGetUniformLocation(m_programID, "m_Kd");
        sh->m_Ka = glGetUniformLocation(m_programID, "m_Ka");
        sh->m_Ks = glGetUniformLocation(m_programID, "m_Ks");
        sh->m_Matopt = glGetUniformLocation(m_programID, "m_Matopt");
        sh->m_Ns = glGetUniformLocation(m_programID, "m_Ns");
    }
}

Shader::~Shader() {
    glDeleteProgram(m_programID);
    free(m_shaderData);
}

void Shader::apply() {
    glUseProgram(m_programID);
}

void Shader::setMaterial1i(MaterialShaderData::MaterialOption opt, int v) {
    GLuint loc = 0;
    MaterialShaderData *matData = (MaterialShaderData *) m_shaderData;

    switch (opt) {
    case MaterialShaderData::MAT_map_Kd:
        loc = matData->m_map_Kd;
        break;
    case MaterialShaderData::MAT_map_Bump:
        loc = matData->m_map_Bump;
        break;
    case MaterialShaderData::MAT_Matopt:
        loc = matData->m_Matopt;
        break;
    default:
        return;
    }

    glUniform1i(loc, v);
}

void Shader::setMaterial1f(MaterialShaderData::MaterialOption opt, GLfloat v) {
    GLuint loc = 0;
    MaterialShaderData *matData = (MaterialShaderData *) m_shaderData;

    switch (opt) {
    case MaterialShaderData::MAT_Ns:
        loc = matData->m_Ns;
        break;
    default:
        return;
    }

    glUniform1f(loc, v);
}

void Shader::setMaterial3f(MaterialShaderData::MaterialOption opt, const glm::vec3 &v) {
    GLuint loc = 0;
    MaterialShaderData *matData = (MaterialShaderData *) m_shaderData;

    switch (opt) {
    case MaterialShaderData::MAT_Ka:
        loc = matData->m_Ka;
        break;
    case MaterialShaderData::MAT_Kd:
        loc = matData->m_Kd;
        break;
    case MaterialShaderData::MAT_Ks:
        loc = matData->m_Ks;
        break;
    default:
        return;
    }

    glUniform3f(loc, v.x, v.y, v.z);
}

static bool loadShaderSingle(const std::string &filename, GLenum shaderType, GLuint &shaderID) {
    std::string shaderCode;
    std::ifstream file(filename);
    GLint errorLength;
    GLint status;

    if (!file) {
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
