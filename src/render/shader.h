#pragma once
#include <GL/glew.h>
#include <string>

namespace Shader {

    void addDefinition(const std::string &name, const std::string &value);
    template<typename T> void addDefinition(const std::string &name, T v) {
        addDefinition(name, std::to_string(v));
    }

    /**
     * @brief Compiles and links shader program
     * @param program - result program ID
     * @param count - count of shaders to load
     * @example loadProgram(p, 2, GL_VERTEX_SHADER, ".../shader.vert", GL_FRAGMENT_SHADER, ".../shader.frag")
     */
    bool loadProgram(GLuint &program, int count, ...);

    /**
     * @brief Compile single shader source without linking it to any program
     * @param shaderType - shader type, as accepted by glCreateShader
     * @param path - shader source file path
     * @param shaderID - result shader ID
     */
    bool loadShader(GLenum shaderType, const char *path, GLuint &shaderID);

}
