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
    void addChild(GameObject *child);

    void setLocalPosition(glm::vec3 pos);
    void setWorldPosition(glm::vec3 pos);
    void translate(glm::vec3 dt);

    virtual void onUpdatePosition();

    const glm::vec3 &getWorldPosition() const;

    GameObject *m_parent = nullptr;
private:
    std::list<GameObject *> m_children;
    glm::vec3 m_localPosition, m_worldPosition;
};

class MeshObject: public GameObject {
public:
    MeshObject(ModelMesh m);

    void setScale(glm::vec3 scale);
    void setShader(Shader *sh);
    void render();

    void onUpdatePosition() override;

private:
    Shader *m_shader;
    ModelMesh m_mesh;

    glm::vec3 m_scale = glm::vec3(1, 1, 1);
    glm::mat4 m_modelMatrix;
};
