#pragma once
#include "gameobject.h"
#include "render/model.h"
#include <vector>

class ModelObject: public GameObject {
public:
    struct PartInstance {
        glm::mat4 *modelMatrixStorage = nullptr;
        int baseIndex = -1;
    };

    GAME_OBJECT(ModelObject);

    void recalcMatrix();
    void addPartInstance(int baseIndex, glm::mat4 *modelMatrixStorage);

    void cloneFrom(GameObject *obj) override;

public:
    GAME_PROPERTY_PRIM(Model, Model *, true) = nullptr;
    GAME_PROPERTY(PartInstances, std::vector<PartInstance>, true);
};
