#pragma once
#include <string>
#include <GL/glew.h>
#include <glm/glm.hpp>

//struct MaterialShaderData {
    //GLuint m_Ka, m_Kd, m_Ks, m_map_Kd, m_map_Bump, m_Matopt, m_Ns;

    //enum MaterialOption {
        //MAT_Ka,
        //MAT_Kd,
        //MAT_Ks,
        //MAT_map_Kd,
        //MAT_map_Bump,
        //MAT_Matopt,
        //MAT_Ns
    //};
//};

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
    void initUniforms();

    //void setMaterial3f(MaterialShaderData::MaterialOption opt, const glm::vec3 &v);
    //void setMaterial1i(MaterialShaderData::MaterialOption opt, GLint v);
    //void setMaterial1f(MaterialShaderData::MaterialOption opt, GLfloat v);

    void applyMaterial(Material *mat);

    GLuint getUniformLocation(const std::string &name);

    static Shader *loadShader(const std::string &vertFile, const std::string &fragFile);

private:

    GLuint m_programID;
    ShaderType m_type = SH_MATERIAL;
    GLuint m_materialBufferID;
    //void *m_shaderData = nullptr;
};
