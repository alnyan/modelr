#include "renderer.h"
#include "input.h"

#include <GLFW/glfw3.h>
#include <glm/glm.hpp>

#include "modelobject.h"
#include "resources.h"

int init(void) {
    ModelObject *obj0 = new ModelObject;
    obj0->setModel(getModelObject("model.obj"));

    Renderer::scene->addStaticObject(obj0);
    Renderer::scene->addStaticObject(obj0, { 1, 1, 1 });

    return 0;
}

////

void updatePlayer(double t, double dt) {
    int fwd = Input::get(GLFW_KEY_W) - Input::get(GLFW_KEY_S);
    int strafe = Input::get(GLFW_KEY_A) - Input::get(GLFW_KEY_D);
    int fly = Input::get(GLFW_KEY_Q) - Input::get(GLFW_KEY_Z);

    glm::vec3 walkDir(
        fwd * glm::cos(Renderer::camera->yaw) + strafe * glm::sin(Renderer::camera->yaw),
        fly,
        fwd * glm::sin(Renderer::camera->yaw) - strafe * glm::cos(Renderer::camera->yaw)
    );

    if (fwd != 0 || strafe != 0 || fly != 0) {
        auto walkDelta = 10 * (float) dt * glm::normalize(walkDir);

        Renderer::camera->translate(walkDelta);
    }

    Renderer::camera->recalcMatrix();
}

void update(double t, double dt) {
    updatePlayer(t, dt);
}

void cursorPosCallback(GLFWwindow *win, double x, double y) {
    if (Renderer::width == 0 || Renderer::height == 0) {
        return;
    }

    double cx = (x - Renderer::width / 2) / Renderer::width;
    double cy = (y - Renderer::height / 2) / Renderer::height;

    glfwSetCursorPos(win, Renderer::width / 2, Renderer::height / 2);

    Renderer::camera->yaw += cx;
    Renderer::camera->uniformData.m_cameraDestination = Renderer::camera->uniformData.m_cameraPosition +
        glm::vec4(glm::cos(Renderer::camera->yaw), 0, glm::sin(Renderer::camera->yaw), 0);
}

int main() {
    if (Renderer::init() != 0) {
        return -1;
    }

    Renderer::setCursorCallback(cursorPosCallback);

    Input::listen(GLFW_KEY_W);
    Input::listen(GLFW_KEY_S);
    Input::listen(GLFW_KEY_A);
    Input::listen(GLFW_KEY_D);
    Input::listen(GLFW_KEY_Q);
    Input::listen(GLFW_KEY_Z);

    if (init() != 0) {
        return -1;
    }

    double t0 = glfwGetTime();
    while (!Renderer::shouldClose()) {
        double t = glfwGetTime();
        auto dt = t - t0;
        t0 = t;

        update(t, dt);

        Renderer::render();
        Renderer::pollEvents();
    }

    return 0;
}
