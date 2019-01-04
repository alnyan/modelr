#pragma once
#include <glm/glm.hpp>
#include "gameobject.h"

class Camera: public GameObject {
public:
    Camera(GameObject *parent = nullptr): GameObject(parent) {  }

    virtual void setTarget(glm::vec3 target) = 0;
    virtual glm::vec3 getTarget() = 0;
    virtual const glm::mat4 &getMatrix() = 0;
};

