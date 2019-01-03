#pragma once
#include <glm/glm.hpp>
#include "render/shader.h"
#include "render/model.h"

struct ModelMesh {
    glm::vec3 offset;
    // TODO: rotation
    Model *model;
};

class GameObject {
public:
    GameObject(glm::vec3 pos);

    void addModelMesh(const ModelMesh &m);
    void setShader(Shader *s);
    void render();
    glm::vec3 getPosition() const;

private:
    std::list<ModelMesh> m_models;
    Shader *m_shader = nullptr;
    glm::mat4 m_modelMatrix;
    glm::vec3 m_position;
};
