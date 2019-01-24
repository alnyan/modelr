#include "input.h"
#include <iostream>
#include <map>
#include <set>

static std::set<int> s_listenKeys;
static std::map<int, bool> s_keyStates;

void Input::listen(int k) {
    s_listenKeys.emplace(k);
}

bool Input::get(int k) {
    return s_keyStates[k];
}

void Input::keyCallback(GLFWwindow *win, int key, int scan, int action, int mods) {
    if (s_listenKeys.find(key) != s_listenKeys.end()) {
        s_keyStates[key] = !!action;
    }
}
