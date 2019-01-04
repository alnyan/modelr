#pragma once
#include <glm/glm.hpp>
#include "render/shader.h"
#include <list>
//#include "render/model.h"

//struct ModelMesh {
    //glm::vec3 offset;
    //// TODO: rotation
    //Model *model;
//};

class GameObject {
public:
    GameObject(GameObject *parent = nullptr);

    //void addChild(GameObject *child);

    void setPosition(glm::vec3 pos);
    void setRotation(glm::vec3 rot);

    void translate(glm::vec3 dt);
    void rotate(glm::vec3 dt);

    virtual void onUpdatePosition();
    void onUpdateParent();

    const glm::vec3 &getWorldPosition() const;
    const glm::vec3 &getWorldRotation() const;

    GameObject *m_parent = nullptr;

protected:
    // 3x3 row matrix describing:
    //   pos.x    pos.y    pos.z
    //   rot.x    rot.y    rot.z
    //   scale.x  scale.y  scale.z
    glm::mat3 p_localState, p_worldState;

private:
    std::list<GameObject *> m_children;
};

//class MeshObject: public GameObject {
//public:
    //MeshObject(ModelMesh m);

    //void setScale(glm::vec3 scale);
    //void setShader(Shader *sh);
    //void render();

    //void onUpdatePosition() override;

//private:
    //Shader *m_shader;
    //ModelMesh m_mesh;

    //glm::vec3 m_scale = glm::vec3(1, 1, 1);
    //glm::mat4 m_modelMatrix;
//};
