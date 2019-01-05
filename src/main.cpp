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
#include "fpscamera.h"
#include "render/meshobject.h"

//

static GLFWwindow *s_window;
static Shader *s_shader;

static constexpr float s_moveSpeed = 5;
static double s_walkStart = 0;
static int m_width, m_height;
static double s_lastTime = 0;

static int s_walk = 0;
static int s_strafe = 0;
static int s_fly = 0;

static double s_meanFrameTime = 0;
static double s_frameTimeSum = 0;

static GLuint s_renderMode = GL_TRIANGLES;

static glm::mat4 s_projectionMatrix;

static Scene *s_scene;
static GameObject *s_player;
static Camera *s_camera;

#ifdef RENDER_TO_TEXTURE
static GLuint s_sceneBuffer;
static GLuint s_sceneTextures[2];
static GLuint s_screenArrayID, s_screenBufferID, s_depthBufferID;
static GLfloat s_screenQuadData[] {
    // xyzuv
    -1, -1, 0, 0, 0,
     1, -1, 0, 1, 0,
     1,  1, 0, 1, 1,
     1,  1, 0, 1, 1,
    -1,  1, 0, 0, 1,
    -1, -1, 0, 0, 0
};
static GLuint s_screenShaderID;
#endif

//

int init(void) {
    s_scene = new Scene(s_shader, s_projectionMatrix);

    s_player = new GameObject();
    s_camera = new FPSCamera(s_player);

    s_player->setPosition({ 10, 2, 0 });
    s_camera->setRotation({ 0 * 3.14 / 180, -180 * 3.14 / 180, 0 });

    s_scene->add(s_player);
    s_scene->setActiveCamera(s_camera);
#ifdef RENDER_TO_TEXTURE
    s_scene->setDestinationBuffer(s_sceneBuffer);
#endif

    // Test model
    auto model = Model::loadObj("model.obj");
    if (!model) {
        std::cerr << "Failed to load models" << std::endl;
        return -1;
    }

    for (int i = 0; i < 100; ++i) {
        for (int j = 0; j < 10; ++j) {
            //auto obj = new MeshObject({ glm::vec3(0), model });
            auto obj = new MeshObject(model);
            obj->setPosition({ i * 2, 0, j * 2 });
            obj->setRenderMode(s_renderMode);
            s_scene->add(obj);
        }
    }

    return 0;
}

int setup_gl(void) {
    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LESS);

#ifdef RENDER_TO_TEXTURE
    if (!Shader::loadProgram("screen.vert", "screen.frag", s_screenShaderID)) {
        std::cerr << "Failed to load screen shader" << std::endl;
        return -1;
    }
#endif

    s_shader = Shader::loadShader("shader.vert", "shader.frag");
    if (!s_shader) {
        std::cerr << "Failed to load shaders" << std::endl;
        return -1;
    }
    s_projectionMatrix = glm::perspective(glm::radians(70.0f), 4.0f / 3.0f, 0.1f, 1000.0f);

#ifdef RENDER_TO_TEXTURE
    glGenFramebuffers(1, &s_sceneBuffer);
    glGenTextures(2, s_sceneTextures);
    glGenRenderbuffers(1, &s_depthBufferID);

    glBindFramebuffer(GL_FRAMEBUFFER, s_sceneBuffer);

    glBindTexture(GL_TEXTURE_2D, s_sceneTextures[0]);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, 800, 600, 0, GL_RGB, GL_UNSIGNED_BYTE, 0);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glBindTexture(GL_TEXTURE_2D, s_sceneTextures[1]);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT24, 800, 600, 0, GL_DEPTH_COMPONENT, GL_FLOAT, 0);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glBindTexture(GL_TEXTURE_2D, 0);

    glFramebufferTexture(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, s_sceneTextures[0], 0);
    glFramebufferTexture(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, s_sceneTextures[1], 0);
    GLenum db[] { GL_COLOR_ATTACHMENT0 };
    glDrawBuffers(1, db);

    glGenVertexArrays(1, &s_screenArrayID);
    glGenBuffers(1, &s_screenBufferID);

    glBindVertexArray(s_screenArrayID);
    glBindBuffer(GL_ARRAY_BUFFER, s_screenBufferID);
    glBufferData(GL_ARRAY_BUFFER, sizeof(s_screenQuadData), s_screenQuadData, GL_STATIC_DRAW);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(GLfloat) * 5, 0);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, sizeof(GLfloat) * 5, (GLvoid *) (sizeof(GLfloat) * 3));
    glBindVertexArray(0);

    if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
        std::cerr << "Failed to create framebuffer" << std::endl;
        return -1;
    }
#endif

    glClearColor(0, 0.25, 0.25, 1);


    s_lastTime = glfwGetTime();

    return 0;
}

void render(void) {
    auto t = glfwGetTime();
    auto dt = t - s_lastTime;
    s_lastTime = t;

    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
#ifdef RENDER_TO_TEXTURE
    glBindTexture(GL_TEXTURE_2D, 0);
#endif

    // Move camera
    double wt = t - s_walkStart;
    double bobf = 0.05 * sin(wt * 10) * s_walk;
    double ry = s_camera->getWorldRotation().y;
    glm::vec3 delta(
        s_moveSpeed * (sin(ry) * dt * s_walk - sin(ry + M_PI / 2) * dt * s_strafe),
        s_moveSpeed * s_fly * dt,
        -s_moveSpeed * (cos(ry) * dt * s_walk - cos(ry + M_PI / 2) * dt * s_strafe)
    );

    s_player->translate(delta);

    s_scene->render();
    Model::unbindAll();
    Material::unbindAll();

#ifdef RENDER_TO_TEXTURE
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glViewport(0, 0, m_width, m_height);
    glUseProgram(s_screenShaderID);
    auto l = glGetUniformLocation(s_screenShaderID, "mDepthTexture");
    glUniform1i(l, 1);
    glBindTextures(0, 2, s_sceneTextures);

    glBindVertexArray(s_screenArrayID);
    glEnableVertexAttribArray(0);
    glEnableVertexAttribArray(1);
    glDrawArrays(GL_TRIANGLES, 0, 6);
    glDisableVertexAttribArray(0);
    glDisableVertexAttribArray(1);
    glBindVertexArray(0);
#endif

    auto r = glfwGetTime();

    s_frameTimeSum += r - t;
}

//

void windowSizeCallback(GLFWwindow *win, int width, int height) {
    glViewport(0, 0, width, height);
    m_width = width;
    m_height = height;

#ifdef RENDER_TO_TEXTURE
    glBindTexture(GL_TEXTURE_2D, s_sceneTextures[0]);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, m_width, m_height, 0, GL_RGB, GL_UNSIGNED_BYTE, 0);
    glBindTexture(GL_TEXTURE_2D, s_sceneTextures[1]);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT24, m_width, m_height, 0, GL_DEPTH_COMPONENT, GL_FLOAT, 0);
    glBindTexture(GL_TEXTURE_2D, 0);
    s_scene->setViewport(m_width, m_height);
#endif

    s_scene->setProjectionMatrix(glm::perspective(glm::radians(45.0f), ((float) width) / ((float) height), 0.1f, 100.0f));
}

void cursorPosCallback(GLFWwindow *win, double x, double y) {
    double cx = (x - m_width / 2) / m_width;
    double cy = (y - m_height / 2) / m_height;

    glfwSetCursorPos(win, m_width / 2, m_height / 2);

    s_camera->rotate(glm::vec3(cy * 1.5, cx * 1.5, 0));
}

void keyCallback(GLFWwindow *win, int key, int scan, int action, int mods) {
    if (key == GLFW_KEY_W) {
        s_walk = !!action;
        if (action == 1) {
            s_walkStart = glfwGetTime();
        }
    }
    if (key == GLFW_KEY_S) {
        s_walk = -!!action;
        if (action == 1) {
            s_walkStart = glfwGetTime();
        }
    }
    if (key == GLFW_KEY_A) {
        s_strafe = !!action;
        if (action == 1) {
            s_walkStart = glfwGetTime();
        }
    }
    if (key == GLFW_KEY_D) {
        s_strafe = -!!action;
        if (action == 1) {
            s_walkStart = glfwGetTime();
        }
    }
    if (key == GLFW_KEY_SPACE) {
        s_fly = !!action;
    }
    if (key == GLFW_KEY_LEFT_SHIFT) {
        s_fly = -!!action;
    }

    if (key == GLFW_KEY_M && action == 1) {
        if (s_renderMode == GL_TRIANGLES) {
            s_renderMode = GL_LINES;
        } else {
            s_renderMode = GL_TRIANGLES;
        }

        for (auto &obj: s_scene->meshObjects) {
            obj->setRenderMode(s_renderMode);
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
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 4);

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

    if (setup_gl() != 0 || init() != 0) {
        glfwTerminate();
        return -1;
    }
    auto t0 = glfwGetTime();
    int frames = 0;

    while (!glfwWindowShouldClose(s_window)) {
        auto t1 = glfwGetTime();
        if (t1 - t0 > 1) {
            std::cout << frames << " frames" << std::endl;
            std::cout << s_frameTimeSum << " sec" << std::endl;
            std::cout << (s_frameTimeSum / frames) << " s/f" << std::endl;
            t0 = t1;
            frames = 0;
            s_frameTimeSum = 0;
        }

        render();
        ++frames;

        glfwSwapBuffers(s_window);
        glfwPollEvents();
    }

    glfwTerminate();

    return 0;
}
