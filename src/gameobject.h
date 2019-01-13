#pragma once
#include <glm/gtc/matrix_transform.hpp>
#include <glm/glm.hpp>

struct GameObject {
    glm::vec3 pos, vel;
    int dataIndex;

    glm::mat4 getMatrix() const {
        return glm::translate(glm::mat4(1), pos);
    }
};
