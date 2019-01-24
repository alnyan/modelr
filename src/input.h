#pragma once
#include <GL/glew.h>
#include <GLFW/glfw3.h>

namespace Input {

    void keyCallback(GLFWwindow *win, int key, int scan, int action, int mods);

    void listen(int key);
    bool get(int key);

}
