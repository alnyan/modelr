#include "modelobject.h"

GAME_CONSTRUCTOR(ModelObject) {}

Model *ModelObject::getModel() const {
    return m_Model;
}

template<> void ModelObject::setModel(Model *m) {
    m_PartInstances.clear();
    m_Model = m;
}

void ModelObject::cloneFrom(GameObject *obj) {
    GameObject::cloneFrom(obj);

    if (ModelObject *mod = dynamic_cast<ModelObject *>(obj)) {
        m_Model = mod->m_Model;
    }
}

void ModelObject::addPartInstance(int baseIndex, glm::mat4 *modelMatrixStorage) {
    setCloneable(false);

    m_PartInstances.push_back({
        modelMatrixStorage, baseIndex
    });
}

const std::vector<ModelObject::PartInstance> &ModelObject::getPartInstances() const {
    return m_PartInstances;
}
