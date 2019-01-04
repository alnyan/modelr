#pragma once
#include "camera.h"

class FPSCamera: public Camera {
public:
    FPSCamera(GameObject *parent = nullptr);

    const glm::mat4 &getMatrix() override;
    glm::vec3 getTarget() override;
    void setTarget(glm::vec3 dst) override;

    void onUpdatePosition() override;

private:
    glm::vec3 m_target;
    glm::mat4 m_matrix;
};
