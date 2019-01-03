#include "scene.h"
#include <glm/gtc/matrix_transform.hpp>

const glm::mat4 &Camera::getMatrix() const {
    return m_matrix;
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

Scene::Scene(Shader *s, glm::mat4 p): m_projectionMatrix{p}, m_shader{s} {
}

void Scene::render() {
    m_shader->apply();

    const auto &cameraMatrix = m_camera.getMatrix();
    auto cameraPos = m_camera.getWorldPosition();
    auto l = m_shader->getUniformLocation("mProjectionMatrix");
    glUniformMatrix4fv(l, 1, GL_FALSE, &m_projectionMatrix[0][0]);
    l = m_shader->getUniformLocation("mCameraPosition");
    glUniform3f(l, cameraPos.x, cameraPos.y, cameraPos.z);
    l = m_shader->getUniformLocation("mCameraMatrix");
    glUniformMatrix4fv(l, 1, GL_FALSE, &cameraMatrix[0][0]);
    l = m_shader->getUniformLocation("mCameraDestination");
    glUniform3f(l, m_camera.dst.x, m_camera.dst.y, m_camera.dst.z);

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
