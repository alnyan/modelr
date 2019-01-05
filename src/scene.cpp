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

#ifdef RENDER_TO_TEXTURE
void Scene::setViewport(int w, int h) {
    m_width = w;
    m_height = h;
}

void Scene::setDestinationBuffer(GLuint fbo) {
    m_destinationBuffer = fbo;
}
#endif

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

#ifdef RENDER_TO_TEXTURE
    glBindFramebuffer(GL_FRAMEBUFFER, m_destinationBuffer);
    glViewport(0, 0, m_width, m_height);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
#endif

    m_shader->apply();

    m_sceneUniformData.m_cameraMatrix = m_activeCamera->getMatrix();
    m_sceneUniformData.m_cameraPosition = glm::vec4(m_activeCamera->getWorldPosition(), 1);
    m_sceneUniformData.m_cameraDestination = glm::vec4(m_activeCamera->getTarget(), 1);

    // Update everything except projection matrix
    glNamedBufferSubData(m_sceneUniformBufferID, sizeof(glm::mat4), sizeof(m_sceneUniformData) - sizeof(glm::mat4),
            (void *) ((uintptr_t) &m_sceneUniformData + sizeof(glm::mat4)));

    for (const auto &o: meshObjects) {
        o->render();
    }

#ifdef RENDER_TO_TEXTURE
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
#endif
}

void Scene::add(GameObject *o) {
    allObjects.push_back(o);
    if (MeshObject *m = dynamic_cast<MeshObject *>(o)) {
        m->setShader(m_shader);
        meshObjects.push_back(m);
    }
}

