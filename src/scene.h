#pragma once
#include <glm/glm.hpp>
#include "gameobject.h"
#include "camera.h"
#include "render/shader.h"
#include "render/meshobject.h"

class Scene {
public:
    Scene(Shader *s, glm::mat4 projection);

    void add(GameObject *);
    void render();
    void setActiveCamera(Camera *c);

    void setProjectionMatrix(glm::mat4 m);

private:

    struct SceneUniformData {
        glm::mat4 m_projectionMatrix;
        glm::mat4 m_cameraMatrix;
        glm::vec4 m_cameraPosition;
        glm::vec4 m_cameraDestination;
    } m_sceneUniformData;

    Shader *m_shader;

    std::list<GameObject *> m_allObjects;
    // For faster drawing
    std::list<MeshObject *> m_meshObjects;

    Camera *m_activeCamera = nullptr;

    GLuint m_sceneUniformBufferID;
};
