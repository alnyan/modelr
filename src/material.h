#pragma once
#include "texture.h"
#include "shader.h"
#include <glm/glm.hpp>

class Material {
public:
    ~Material();

    void apply(Shader *shader);

    static Material *loadMtl(const std::string &path);

private:
    Texture *m_map_Kd = nullptr;
    glm::vec3 m_Ka = { 0.2f, 0.2f, 0.2f },
              m_Kd = { 0.8f, 0.8f, 0.8f },
              m_Ks = { 1.0f, 1.0f, 1.0f };
    float m_Ns;
};
