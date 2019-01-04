#include "scene.h"
#include <glm/gtc/matrix_transform.hpp>
#include <iostream>

Scene::Scene(Shader *s, glm::mat4 p): m_shader{s} {
    m_sceneUniformData.m_projectionMatrix = p;

    glGenBuffers(1, &m_sceneUniformBufferID);
    glBindBuffer(GL_UNIFORM_BUFFER, m_sceneUniformBufferID);
    glBufferData(GL_UNIFORM_BUFFER, sizeof(m_sceneUniformData), &m_sceneUniformData, GL_DYNAMIC_DRAW);
    glBindBuffer(GL_UNIFORM_BUFFER, 0);

    glBindBufferBase(GL_UNIFORM_BUFFER, 0, m_sceneUniformBufferID);
}

void Scene::setProjectionMatrix(glm::mat4 m) {
    m_sceneUniformData.m_projectionMatrix = m;
    glNamedBufferSubData(m_sceneUniformBufferID, 0, sizeof(glm::mat4), &m_sceneUniformData);
}

void Scene::setActiveCamera(Camera *c) {
    m_activeCamera = c;
}

void Scene::render() {
    if (!m_activeCamera) {
        return;
    }

    m_shader->apply();

    m_sceneUniformData.m_cameraMatrix = m_activeCamera->getMatrix();
    m_sceneUniformData.m_cameraPosition = glm::vec4(m_activeCamera->getWorldPosition(), 1);
    m_sceneUniformData.m_cameraDestination = glm::vec4(m_activeCamera->getTarget(), 1);

    //glm::vec3 p0(5, 5, 5), p1(0, 0, 0);
    //m_sceneUniformData.m_cameraMatrix = glm::lookAt(p0, p1, glm::vec3(0, 1, 0));
    //m_sceneUniformData.m_cameraPosition = glm::vec4(p0, 1);
    //m_sceneUniformData.m_cameraDestination = glm::vec4(p1, 1);

    // Update everything except projection matrix
    glNamedBufferSubData(m_sceneUniformBufferID, sizeof(glm::mat4), sizeof(m_sceneUniformData) - sizeof(glm::mat4),
            (void *) ((uintptr_t) &m_sceneUniformData + sizeof(glm::mat4)));

    for (const auto &o: m_meshObjects) {
        o->render();
    }
}

void Scene::add(GameObject *o) {
    m_allObjects.push_back(o);
    if (MeshObject *m = dynamic_cast<MeshObject *>(o)) {
        m->setShader(m_shader);
        m_meshObjects.push_back(m);
    }
}

