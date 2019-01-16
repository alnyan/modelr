#include "gameobject.h"

GameObject::GameObject(GameObject *parent, glm::vec3 pos, glm::quat rot):
    m_Parent {parent},
    m_LocalPosition {pos},
    m_LocalRotation {rot} {}

GameObject::OptimizationClass GameObject::getOptimizationClass() const {
    return m_OptimizationClass;
}

template<> void GameObject::setOptimizationClass(GameObject::OptimizationClass c) {
    m_OptimizationClass = c;
}

template<> void GameObject::setCloneable(bool v) {
    m_Cloneable = v;
}

void GameObject::update(double t, double dt) {
}

void GameObject::cloneFrom(GameObject *other) {
    m_IsClone = true;
    m_OptimizationClass = other->m_OptimizationClass;
    m_PhysicsEnabled = other->m_PhysicsEnabled;
    m_Cloneable = other->m_Cloneable;
    m_Velocity = other->m_Velocity;
    m_Torque = other->m_Torque;
    m_Parent = other->m_Parent;
    m_LocalPosition = other->m_LocalPosition;
    m_LocalRotation = other->m_LocalRotation;
}
