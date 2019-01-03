#include "scene.h"
#include <glm/gtc/matrix_transform.hpp>

const glm::mat4 &Camera::getMatrix() const {
    return m_matrix;
}

void Camera::translate(glm::vec3 dt) {
    src += dt;
    if (!m_mode) {
        dst += dt;
    }
    updateMatrix();
}

void Camera::lookAt(glm::vec3 f, glm::vec3 t) {
    src = f;
    dst = t;
    updateMatrix();
}

void Camera::rotate(glm::vec3 r) {
    dst += r;
    updateMatrix();
}

void Camera::setPos(glm::vec3 p) {
    src = p;
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
        translate = glm::translate(translate, -src);

        m_matrix = rotate * translate;
    } else {
        m_matrix = glm::lookAt(src, dst, glm::vec3(0, 1, 0));
    }
}

Scene::Scene(Shader *s, glm::mat4 p): m_projectionMatrix{p}, m_shader{s} {
}

void Scene::render() {
    m_shader->apply();

    const auto &cameraMatrix = m_camera.getMatrix();
    auto l = m_shader->getUniformLocation("mProjectionMatrix");
    glUniformMatrix4fv(l, 1, GL_FALSE, &m_projectionMatrix[0][0]);
    l = m_shader->getUniformLocation("mCameraPosition");
    glUniform3f(l, m_camera.src.x, m_camera.src.y, m_camera.src.z);
    l = m_shader->getUniformLocation("mCameraMatrix");
    glUniformMatrix4fv(l, 1, GL_FALSE, &cameraMatrix[0][0]);
    l = m_shader->getUniformLocation("mCameraDestination");
    glUniform3f(l, m_camera.dst.x, m_camera.dst.y, m_camera.dst.z);

    for (const auto &o: m_objects) {
        o->render();
    }
}

void Scene::add(GameObject *o) {
    o->setShader(m_shader);
    m_objects.push_back(o);
}

Camera &Scene::camera() {
    return m_camera;
}
