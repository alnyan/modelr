#pragma once
#include "texture.h"
#include "shader.h"
#include <glm/glm.hpp>

struct MaterialUniformObject {
    glm::vec4 m_Ka, m_Kd, m_Ks;
    GLfloat m_Ns;
    int m_Matopt;
    int m_pad[2];
};

class Material {
public:
    ~Material();

    void apply(Shader *shader);

    static void unbindAll();
    static Material *loadMtl(const std::string &path);

    MaterialUniformObject uniformData;
private:
    Texture *m_map_Kd = nullptr;
    Texture *m_map_Bump = nullptr;
};
