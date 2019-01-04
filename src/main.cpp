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
static Shader *s_shader;

static constexpr float s_moveSpeed = 2;
static double s_walkStart = 0;
static int m_width, m_height;
static bool s_wDown = false;
static double s_lastTime = 0;

static glm::mat4 s_projectionMatrix;

static Scene *s_scene;
static GameObject *s_player;

//

void init(void) {
    s_scene = new Scene(s_shader, s_projectionMatrix);

    s_player = new GameObject();
    s_player->addChild(&s_scene->camera());

    s_player->setWorldPosition({ 5, 2, 5 });
    s_scene->camera().setRotation({ 0, 0, 0 });

    s_scene->add(s_player);


    // Test model
    auto model = Model::loadObj("model.obj");

    for (int i = 0; i < 100; ++i) {
        for (int j = 0; j < 100; ++j) {
            auto obj = new MeshObject({ glm::vec3(0), model });
            obj->setWorldPosition({ i * 2, 0, j * 2 });
            s_scene->add(obj);
        }
    }
}

void setup_gl(void) {
    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LESS);

    s_shader = Shader::loadShader("shader.vert", "shader.frag");
    s_projectionMatrix = glm::perspective(glm::radians(70.0f), 4.0f / 3.0f, 0.1f, 1000.0f);

    glClearColor(0, 0.25, 0.25, 1);

    s_lastTime = glfwGetTime();
}

void render(void) {
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    auto t = glfwGetTime();
    auto dt = t - s_lastTime;
    s_lastTime = t;

    // Move camera
    double wt = t - s_walkStart;
    double bobf = 0.05 * sin(wt * 10) * s_wDown;
    double ry = s_scene->camera().dst.y;
    float dx = s_moveSpeed * sin(ry) * dt * s_wDown;
    float dz = -s_moveSpeed * cos(ry) * dt * s_wDown;
    glm::vec3 delta(dx, 0, dz);
    glm::vec3 bob(-bobf * sin(ry + M_PI / 2), bobf, bobf * cos(ry + M_PI / 2));

    s_scene->camera().bob(bob);
    s_player->translate(delta);

    s_scene->render();
    Model::unbindAll();
    Material::unbindAll();
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
        if (action == 1) {
            s_walkStart = glfwGetTime();
        }
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
    init();
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
