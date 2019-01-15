#pragma once
#include <glm/gtc/matrix_transform.hpp>
#include <glm/glm.hpp>
#include <list>

struct GameObject {
    glm::vec3 pos, vel, rot;
    std::list<int> dataIndices;
    bool isStatic = false;

    glm::mat4 getMatrix() const {
        return glm::translate(glm::mat4(1), pos);
    }
};
