#pragma once
#include <glm/glm.hpp>
#include "../gameobject.h"
#include "model.h"

class MeshObject: public GameObject {
public:
    MeshObject(Model *m, GameObject *parent = nullptr);

    bool compare(MeshObject *o) const;

    void onUpdatePosition() override;

    void render();
    void setShader(Shader *s);

private:
    Shader *m_shader;
    Model *m_model;
    glm::mat4 m_modelMatrix;
};
