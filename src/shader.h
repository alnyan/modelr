#pragma once
#include <string>
#include <GL/glew.h>

class Shader {
public:
    Shader(GLuint programID);
    ~Shader();

    void apply();
    GLuint getUniformLocation(const std::string &name);

    static Shader *loadShader(const std::string &vertFile, const std::string &fragFile);

private:

    GLuint m_programID;
};
