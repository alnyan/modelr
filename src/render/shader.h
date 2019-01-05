#pragma once
#include <string>
#include <GL/glew.h>
#include <glm/glm.hpp>

class Material;

class Shader {
public:
    enum ShaderType {
        SH_MATERIAL,
        SH_WORLD
    };

    Shader(GLuint programID);
    ~Shader();

    void apply();

    void applyMaterial(Material *mat);

    GLuint getUniformLocation(const std::string &name);

    static Shader *loadShader(const std::string &vertFile, const std::string &fragFile);
    static bool loadProgram(const std::string &vertFile, const std::string &fragFile, GLuint &p);

private:

    GLuint m_programID;
    ShaderType m_type = SH_MATERIAL;
    GLuint m_materialBufferID;
};
