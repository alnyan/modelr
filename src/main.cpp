#include <iostream>
#include <math.h>
#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include "model.h"
#include "shader.h"
#include "texture.h"
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

//

static GLFWwindow *s_window;
static GLuint s_vertexArrayID;
static Model *s_model;
static GLuint s_vertexBufferID, s_texCoordBufferID;
static Shader *s_shader;
static Texture *s_texture;

static glm::vec3 s_cameraPos, s_cameraRot;
static glm::mat4 s_viewMatrix, s_projectionMatrix;

//

void setup_gl(void) {
    glGenVertexArrays(1, &s_vertexArrayID);
    glBindVertexArray(s_vertexArrayID);

    const float plane[] = {
        -1, -1, -2,
        1, -1, -2,
        1, 1, -2
    };
    const float texPlane[] = {
        0, 0,
        1, 0,
        1, 1
    };

    glGenBuffers(1, &s_vertexBufferID);
    glBindBuffer(GL_ARRAY_BUFFER, s_vertexBufferID);
    glBufferData(GL_ARRAY_BUFFER, sizeof(plane), plane, GL_STATIC_DRAW);

    glGenBuffers(1, &s_texCoordBufferID);
    glBindBuffer(GL_ARRAY_BUFFER, s_texCoordBufferID);
    glBufferData(GL_ARRAY_BUFFER, sizeof(texPlane), texPlane, GL_STATIC_DRAW);

    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LESS);

    s_model = Model::loadObj("model.obj");
    s_texture = Texture::loadPng("texture.png");
    s_shader = Shader::loadShader("shader.vert", "shader.frag");

    s_projectionMatrix = glm::perspective(glm::radians(45.0f), 4.0f / 3.0f, 0.1f, 100.0f);
}

void render(void) {
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    if (s_texture) {
        s_texture->bind();
    }
    s_shader->apply();

    auto t = glfwGetTime();

    glm::vec3 eyePos(10 * cos(t), 5, 10 * sin(t));
    glm::vec3 eyeDst(0, 0, 0);

    s_viewMatrix = glm::lookAt(
        eyePos,
        eyeDst,
        glm::vec3(0, 1, 0)
    );

    auto resMatrix = s_projectionMatrix * s_viewMatrix;
    auto l = s_shader->getUniformLocation("mMatrix");
    glUniformMatrix4fv(l, 1, GL_FALSE, &resMatrix[0][0]);

    l = s_shader->getUniformLocation("mCameraPosition");
    glUniform3f(l, eyePos.x, eyePos.y, eyePos.z);

    l = s_shader->getUniformLocation("mCameraDestination");
    glUniform3f(l, eyeDst.x, eyeDst.y, eyeDst.z);

    l = s_shader->getUniformLocation("texSampler");
    glUniform1i(l, GL_TEXTURE_2D);

    s_model->render(s_shader);

    glEnableVertexAttribArray(0);
    glEnableVertexAttribArray(2);
    glBindBuffer(GL_ARRAY_BUFFER, s_vertexBufferID);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, nullptr);
    glBindBuffer(GL_ARRAY_BUFFER, s_texCoordBufferID);
    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 0, nullptr);
    glDrawArrays(GL_TRIANGLES, 0, 3);
    glDisableVertexAttribArray(0);
    glDisableVertexAttribArray(2);
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
