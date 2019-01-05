#include "meshobject.h"
#include <glm/gtc/matrix_transform.hpp>
#include <iostream>

MeshObject::MeshObject(Model m, GameObject *parent): GameObject(parent), m_model{m} {}

void MeshObject::onUpdatePosition() {
    m_modelMatrix = glm::translate(glm::mat4(1), p_worldState[0]);
}

//void MeshObject::setShader(Shader *s) {
    //m_shader = s;
//}

void MeshObject::setRenderMode(GLuint mode) {
    m_renderMode = mode;
}

//void MeshObject::render() {
    //auto l = m_shader->getUniformLocation("mModelMatrix");
    //glUniformMatrix4fv(l, 1, GL_FALSE, &m_modelMatrix[0][0]);

    //m_model->bind(m_shader);
    //m_model->render(m_shader, m_renderMode);
//}
