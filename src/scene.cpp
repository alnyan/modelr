#include "scene.h"
#include <iostream>

void Scene::init() {
    staticModelObjectBuffers.generate();
    dynamicModelObjectBuffers.generate();
}

int Scene::addPartToBuffers(DynamicBufferGroup *group, const Part *p) {
    int index = group->getSize();
    group->indirectCommands.append({ p->size, 1, p->begin, 0 });
    group->meshAttribs.append({ p->materialIndex, {}, glm::vec4(0) });
    group->modelMatrices.append(glm::mat4(1));
    return index;
}


void Scene::addStaticObject(ModelObject *obj, glm::vec3 pos, glm::quat rot) {
    assert(obj);
    assert(obj->getModel());

    auto instance = cloneGameObject(obj);
    instance->setLocalPosition(pos);
    instance->setLocalRotation(rot);

    // For dynamic objects, use indirect draw buffers
    // Add all parts
    for (const auto &part: instance->getModel()->parts) {
        int index = addPartToBuffers(&staticModelObjectBuffers, &part);
        instance->addPartInstance(index, &staticModelObjectBuffers.modelMatrices[index]);
    }

    assert(instance->getPartInstances().size() == instance->getModel()->parts.size());

    instance->recalcMatrix();

    staticModelObjects.push_back(instance);
}

void Scene::addDynamicObject(ModelObject *obj, glm::vec3 pos, glm::quat rot) {
    assert(obj);
    assert(obj->getModel());

    auto instance = cloneGameObject(obj);
    instance->setLocalPosition(pos);
    instance->setLocalRotation(rot);

    // For dynamic objects, use indirect draw buffers
    // Add all parts
    for (const auto &part: instance->getModel()->parts) {
        int index = addPartToBuffers(&dynamicModelObjectBuffers, &part);
        instance->addPartInstance(index, &dynamicModelObjectBuffers.modelMatrices[index]);
    }

    assert(instance->getPartInstances().size() == instance->getModel()->parts.size());

    instance->recalcMatrix();

    dynamicModelObjects.push_back(instance);
}
