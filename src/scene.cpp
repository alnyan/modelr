#include "scene.h"
#include <glm/gtc/matrix_transform.hpp>
#include <iostream>

void Camera::setMode(bool m) {
    m_mode = m;
    updateMatrix();
}

void Camera::onUpdatePosition() {
    updateMatrix();
}

void Camera::bob(glm::vec3 bx) {
    b = bx;
}

void Camera::lookAt(glm::vec3 f, glm::vec3 t) {
    dst = t;
    setWorldPosition(f);
}

void Camera::rotate(glm::vec3 r) {
    dst += r;
    updateMatrix();
}

void Camera::setRotation(glm::vec3 r) {
    dst = r;
    updateMatrix();
}

void Camera::updateMatrix() {
    if (m_mode) {
        glm::mat4 matPitch = glm::mat4(1.0f);//identity matrix
        glm::mat4 matYaw = glm::mat4(1.0f);//identity matrix
        matPitch = glm::rotate(matPitch, dst.x, glm::vec3(1.0f, 0.0f, 0.0f));
        matYaw = glm::rotate(matYaw, dst.y, glm::vec3(0.0f, 1.0f, 0.0f));

        glm::mat4 rotate = matPitch * matYaw;

        glm::mat4 translate = glm::mat4(1.0f);
        translate = glm::translate(translate, -getWorldPosition() + b);

        m_matrix = rotate * translate;
    } else {
        m_matrix = glm::lookAt(getWorldPosition() + b, dst + b, glm::vec3(0, 1, 0));
    }
}

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

void Scene::render() {
    m_shader->apply();

    auto cameraPos = m_camera.getWorldPosition();
    m_sceneUniformData.m_cameraMatrix = m_camera.m_matrix;
    m_sceneUniformData.m_cameraPosition = glm::vec4(cameraPos, 1);
    m_sceneUniformData.m_cameraDestination = glm::vec4(m_camera.dst, 1);

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

Camera &Scene::camera() {
    return m_camera;
}
