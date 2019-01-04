#include "gameobject.h"
#include <assert.h>
#include <glm/gtc/matrix_transform.hpp>
#include <iostream>

void GameObject::addChild(GameObject *child) {
    m_children.push_back(child);
    child->m_parent = this;
}

void GameObject::onUpdatePosition() {
}

void GameObject::setLocalPosition(glm::vec3 p) {
    auto oldLocalPos = m_localPosition;
    m_localPosition = p;
    m_worldPosition += m_localPosition - oldLocalPos;
    onUpdatePosition();

    for (auto &child: m_children) {
        child->m_worldPosition = child->m_localPosition + m_worldPosition;
        child->onUpdatePosition();
    }
}

void GameObject::setWorldPosition(glm::vec3 p) {
    auto oldWorldPos = m_worldPosition;
    m_worldPosition = p;
    m_localPosition = m_parent ? (m_parent->m_worldPosition - oldWorldPos) : glm::vec3(0);
    onUpdatePosition();

    for (auto &child: m_children) {
        child->m_worldPosition = child->m_localPosition + p;
        child->onUpdatePosition();
    }
}

void GameObject::translate(glm::vec3 d) {
    setLocalPosition(m_localPosition + d);
}

const glm::vec3 &GameObject::getWorldPosition() const {
    return m_worldPosition;
}

//const glm::vec3 &GameObject::getLoca

MeshObject::MeshObject(ModelMesh m): m_mesh{m} {
    onUpdatePosition();
}

void MeshObject::setScale(glm::vec3 s) {
    m_scale = s;
    onUpdatePosition();
}

void MeshObject::onUpdatePosition() {
    m_modelMatrix = glm::translate(glm::scale(glm::mat4(1.0f), m_scale), getWorldPosition());
}

void MeshObject::setShader(Shader *s) {
    m_shader = s;
}

void MeshObject::render() {
    assert(m_shader);

    auto l = m_shader->getUniformLocation("mModelMatrix");
    glUniformMatrix4fv(l, 1, GL_FALSE, &m_modelMatrix[0][0]);

    m_mesh.model->bind(m_shader);
    m_mesh.model->render(m_shader);
}
