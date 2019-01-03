#include "gameobject.h"
#include <assert.h>
#include <glm/gtc/matrix_transform.hpp>
#include <iostream>

GameObject::GameObject(glm::vec3 pos): m_position{pos} {
    m_modelMatrix = glm::translate(glm::mat4(1.0f), pos);
}

void GameObject::addModelMesh(const ModelMesh &m) {
    m_models.push_back(m);
}

void GameObject::setShader(Shader *s) {
    m_shader = s;
}

glm::vec3 GameObject::getPosition() const {
    return m_position;
}

void GameObject::render() {
    assert(m_shader != nullptr);

    auto l = m_shader->getUniformLocation("mModelMatrix");
    glUniformMatrix4fv(l, 1, GL_FALSE, &m_modelMatrix[0][0]);

    for (const auto &m: m_models) {
        // TODO: per-model offset
        m.model->bind(m_shader);
        m.model->render(m_shader);
    }
}
