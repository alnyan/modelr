#pragma once
#include <glm/glm.hpp>
#include "gameobject.h"
#include "render/shader.h"
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtc/quaternion.hpp>
#include <glm/gtx/quaternion.hpp>

class Camera: public GameObject {
public:
    const glm::mat4 &getMatrix() const;

    void lookAt(glm::vec3 f, glm::vec3 p);

    void bob(glm::vec3 f);
    void setPos(glm::vec3 f);
    void setRotation(glm::vec3 r);
    void rotate(glm::vec3 r);

    void onUpdatePosition() override;

    void setMode(bool mode);

    glm::vec3 dst, b;
private:
    void updateMatrix();

    bool m_mode = true;
    glm::mat4 m_matrix;
};

class Scene {
public:
    Scene(Shader *s, glm::mat4 projection);

    void add(GameObject *);
    void render();
    Camera &camera();

private:
    Shader *m_shader;

    std::list<GameObject *> m_allObjects;
    // For faster drawing
    std::list<MeshObject *> m_meshObjects;

    Camera m_camera;
    glm::mat4 m_projectionMatrix;
};
