#include "fpscamera.h"
#include <glm/gtc/matrix_transform.hpp>

FPSCamera::FPSCamera(GameObject *parent): Camera(parent) {}

const glm::mat4 &FPSCamera::getMatrix() {
    return m_matrix;
}

glm::vec3 FPSCamera::getTarget() {
    return m_target;
}

void FPSCamera::setTarget(glm::vec3 t) {
    m_target = t;
    m_matrix = glm::lookAt(p_worldState[0], m_target, glm::vec3(0, 1, 0));
}

void FPSCamera::onUpdatePosition() {
    glm::mat4 matPitch(1), matYaw(1);
    matPitch = glm::rotate(matPitch, p_worldState[1].x, glm::vec3(1, 0, 0));
    matYaw = glm::rotate(matYaw, p_worldState[1].y, glm::vec3(0, 1, 0));
    auto rot = matPitch * matYaw;
    m_target = glm::vec3((rot * glm::vec4(1, 0, 0, 0)));
    m_matrix = rot * glm::translate(glm::mat4(1), -p_worldState[0]);
}
