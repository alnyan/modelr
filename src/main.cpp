#include <iostream>
#include <math.h>
#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include "render/model.h"
#include "render/shader.h"
#include "render/texture.h"
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include "gameobject.h"
#include "scene.h"
#include <glm/gtx/euler_angles.hpp>

//

static GLFWwindow *s_window;
static Model *s_model;
static Shader *s_shader;
static Scene *s_scene;

static constexpr float s_moveSpeed = 2;
static int m_width, m_height;
static bool s_wDown = false;
static double s_lastTime = 0;

#define WWW 10
#define HHH 10

static glm::mat4 s_projectionMatrix;

//

void setup_gl(void) {
    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LESS);

    s_model = Model::loadObj("model.obj");
    s_shader = Shader::loadShader("shader.vert", "shader.frag");

    s_projectionMatrix = glm::perspective(glm::radians(45.0f), 4.0f / 3.0f, 0.1f, 100.0f);

    glClearColor(0, 0.25, 0.25, 1);

    s_scene = new Scene(s_shader, s_projectionMatrix);
    for (int i = 0; i < WWW; ++i) {
        for (int j = 0; j < HHH; ++j) {
            GameObject *o = new GameObject(glm::vec3(i * 2, 0, j * 2));
            o->addModelMesh({ glm::vec3(0), s_model });
            s_scene->add(o);
        }
    }

    s_scene->camera().setPos({ 5.0f, 2.0f, 5.0f });
    s_scene->camera().setRotation({ 0, 0, 0 });

    s_lastTime = glfwGetTime();
}

void render(void) {
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    auto t = glfwGetTime();
    auto dt = t - s_lastTime;
    s_lastTime = t;

    // Move camera
    double ry = s_scene->camera().dst.y;
    float dx = s_moveSpeed * sin(ry) * dt * s_wDown;
    float dz = -s_moveSpeed * cos(ry) * dt * s_wDown;

    s_scene->camera().translate(glm::vec3(dx, 0, dz));

    s_scene->render();
    Model::unbindAll();
}

//

void windowSizeCallback(GLFWwindow *win, int width, int height) {
    glViewport(0, 0, width, height);
    m_width = width;
    m_height = height;
    s_projectionMatrix = glm::perspective(glm::radians(45.0f), ((float) width) / ((float) height), 0.1f, 100.0f);
}

void cursorPosCallback(GLFWwindow *win, double x, double y) {
    double cx = (x - m_width / 2) / m_width;
    double cy = (y - m_height / 2) / m_height;

    glfwSetCursorPos(win, m_width / 2, m_height / 2);

    s_scene->camera().rotate(glm::vec3(cy * 1.5, cx * 1.5, 0));
}

void keyCallback(GLFWwindow *win, int key, int scan, int action, int mods) {
    if (key == GLFW_KEY_W) {
        s_wDown = !!action;
    }
}

//

int main() {
    if (!glfwInit()) {
        return -1;
    }

    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    glfwWindowHint(GLFW_SAMPLES, 4);                                    // 4x MSAA
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);

    s_window = glfwCreateWindow(800, 600, "", NULL, NULL);

    if (!s_window) {
        glfwTerminate();
        return -1;
    }

    glfwSetWindowSizeCallback(s_window, windowSizeCallback);
    glfwSetCursorPosCallback(s_window, cursorPosCallback);
    glfwSetKeyCallback(s_window, keyCallback);

    glfwMakeContextCurrent(s_window);

    glewExperimental = true;
    if (glewInit() != 0) {
        glfwTerminate();
        return -2;
    }

    glfwSwapInterval(1);

    setup_gl();
    auto t0 = glfwGetTime();
    int frames = 0;

    while (!glfwWindowShouldClose(s_window)) {
        auto t1 = glfwGetTime();
        if (t1 - t0 > 1) {
            std::cout << frames << " frames" << std::endl;
            t0 = t1;
            frames = 0;
        }

        render();
        ++frames;

        glfwSwapBuffers(s_window);
        glfwPollEvents();
    }

    glfwTerminate();

    return 0;
}
