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

//

static GLFWwindow *s_window;
static Model *s_model;
static Shader *s_shader;
static Texture *s_texture;

#define WWW 100
#define HHH 100

static GameObject *s_objects[WWW * HHH];

static glm::vec3 s_cameraPos, s_cameraRot;
static glm::mat4 s_viewMatrix, s_projectionMatrix;

//

void setup_gl(void) {
    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LESS);

    s_model = Model::loadObj("model.obj");
    s_texture = Texture::loadPng("texture.png");
    s_shader = Shader::loadShader("shader.vert", "shader.frag");

    s_projectionMatrix = glm::perspective(glm::radians(45.0f), 4.0f / 3.0f, 0.1f, 100.0f);

    glClearColor(0, 0.25, 0.25, 1);

    for (int i = 0; i < WWW; ++i) {
        for (int j = 0; j < HHH; ++j) {
            s_objects[i * HHH + j] = new GameObject(glm::vec3(i * 2, 0, j * 2));
            s_objects[i * HHH + j]->addModelMesh({ glm::vec3(0), s_model });
            s_objects[i * HHH + j]->setShader(s_shader);
        }
    }
}

void render(void) {
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    if (s_texture) {
        s_texture->bind();
    }
    s_shader->apply();

    auto t = glfwGetTime();

    glm::vec3 eyeDst(3, 0, 3);
    glm::vec3 eyePos(5 * cos(t), 5, 5 * sin(t));
    eyePos += eyeDst;

    s_viewMatrix = glm::lookAt(
        eyePos,
        eyeDst,
        glm::vec3(0, 1, 0)
    );


    auto l = s_shader->getUniformLocation("mProjectionMatrix");
    glUniformMatrix4fv(l, 1, GL_FALSE, &s_projectionMatrix[0][0]);
    l = s_shader->getUniformLocation("mCameraPosition");
    glUniform3f(l, eyePos.x, eyePos.y, eyePos.z);
    l = s_shader->getUniformLocation("mCameraMatrix");
    glUniformMatrix4fv(l, 1, GL_FALSE, &s_viewMatrix[0][0]);
    l = s_shader->getUniformLocation("mCameraDestination");
    glUniform3f(l, eyeDst.x, eyeDst.y, eyeDst.z);

    auto t0 = glfwGetTime();

    for (const auto &obj: s_objects) {
        obj->render();
    }
    //s_model->bind(s_shader);
    //auto t1 = glfwGetTime();
    //for (int i = 0; i < 10; ++i) {
        //for (int j = 0; j < 10; ++j) {
            //auto modelMatrix = glm::translate(glm::mat4(1.0f), glm::vec3(i * 2.0f, 0, j * 2.0f));
            //l = s_shader->getUniformLocation("mModelMatrix");
            //glUniformMatrix4fv(l, 1, GL_FALSE, &modelMatrix[0][0]);
            //s_model->render(s_shader);
        //}
    //}
    //auto t2 = glfwGetTime();
    //s_model->unbind(s_shader);
    Model::unbindAll();
    auto t3 = glfwGetTime();


    //std::cout << (t3 - t0) * 1000 << std::endl;
}

//

void windowSizeCallback(GLFWwindow *win, int width, int height) {
    glViewport(0, 0, width, height);
    s_projectionMatrix = glm::perspective(glm::radians(45.0f), ((float) width) / ((float) height), 0.1f, 100.0f);
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
