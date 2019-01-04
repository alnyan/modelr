#include "gameobject.h"
#include <assert.h>
#include <glm/gtc/matrix_transform.hpp>
#include <iostream>

GameObject::GameObject(GameObject *parent):
    m_parent{parent},
    p_localState{{0,0,0},{0,0,0},{1,1,1}},
    p_worldState{{0,0,0},{0,0,0},{1,1,1}} {
    if (parent) {
        parent->m_children.push_back(this);
    }
}

void GameObject::onUpdatePosition() {
    for (GameObject *c: m_children) {
        c->onUpdateParent();
    }
}

void GameObject::onUpdateParent() {
    p_worldState = m_parent->p_worldState + p_localState;
    onUpdatePosition();
}

void GameObject::setPosition(glm::vec3 v) {
    if (m_parent) {
        p_localState[0] = v;
        p_worldState = m_parent->p_worldState + p_localState;
    } else {
        p_worldState[0] = v;
    }

    onUpdatePosition();
}

void GameObject::setRotation(glm::vec3 r) {
    if (m_parent) {
        p_localState[1] = r;
        p_worldState = m_parent->p_worldState + p_localState;
    } else {
        p_worldState[1] = r;
    }

    onUpdatePosition();
}

void GameObject::translate(glm::vec3 d) {
    if (m_parent) {
        p_localState[0] += d;
    }

    p_worldState[0] += d;
    onUpdatePosition();
}

void GameObject::rotate(glm::vec3 d) {
    if (m_parent) {
        p_localState[1] += d;
    }

    p_worldState[1] += d;
    onUpdatePosition();
}

const glm::vec3 &GameObject::getWorldPosition() const {
    return p_worldState[0];
}

const glm::vec3 &GameObject::getWorldRotation() const {
    return p_worldState[1];
}
