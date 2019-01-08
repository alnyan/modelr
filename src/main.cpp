#include <iostream>
#include <math.h>
#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include "render/shader.h"
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include "gameobject.h"
//#include "scene.h"
#include "fpscamera.h"
#include "res/lodepng.h"

#include "render/meshobject.h"
#include "render/wavefront.h"

#include "config.h"

//

static GLFWwindow *s_window;
//static Shader *s_shader;

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

typedef struct {
    GLuint count, instanceCount, first, baseInstance;
} DrawArraysIndirectCommand;
struct SceneUniformData {
    glm::mat4 m_projectionMatrix;
    glm::mat4 m_cameraMatrix;
    glm::vec4 m_cameraPosition;
    glm::vec4 m_cameraDestination;
};

struct MeshAttrib {
    int material;
};

static GLuint s_sceneShaderID;
static GLuint s_allGeometryArrayID;
static GLuint s_modelMatrixBufferID, s_meshAttribBufferID, s_indirectCommandBufferID;
static GLuint s_textureHandleBufferID;
static GLuint s_textureIDs[2];
static GLuint64 s_textureHandles[512];
static std::vector<DrawArraysIndirectCommand> s_indirectCommands;
static std::vector<glm::mat4> s_modelMatrices;
static std::vector<MeshAttrib> s_meshAttribs;

static double s_transferTime, s_drawCallTime;

static MeshBuilder *s_allGeometryBuilder;
static Model s_models[3];

static GLuint s_sceneUniformBufferID;
static SceneUniformData s_sceneUniformData;
//static Scene *s_scene;
//static GameObject *s_player;
//static Camera *s_camera;

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

int loadTexture(GLuint id, const char *path) {
    unsigned int w, h;
    std::vector<unsigned char> data;
    if (lodepng::decode(data, w, h, path) != 0) {
        std::cerr << "Failed to load textures" << std::endl;
        return -1;
    }

    glBindTexture(GL_TEXTURE_2D, id);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, w, h, 0, GL_RGBA, GL_UNSIGNED_BYTE, &data[0]);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glBindTexture(GL_TEXTURE_2D, 0);

    return 0;
}

int loadData(void) {
    std::cout << "Loading data" << std::endl;

    if (!Shader::loadProgram(s_sceneShaderID, 2, GL_VERTEX_SHADER, "shader.vert", GL_FRAGMENT_SHADER, "shader.frag")) {
        std::cerr << "Failed to load shaders" << std::endl;
        return -1;
    }

#ifdef RENDER_TO_TEXTURE
    if (!Shader::loadProgram(s_screenShaderID, 2, GL_VERTEX_SHADER, "screen.vert", GL_FRAGMENT_SHADER, "screen.frag")) {
        std::cerr << "Failed to load screen shader" << std::endl;
        return -1;
    }
#endif

    glGenTextures(sizeof(s_textureIDs) / sizeof(s_textureIDs[0]), s_textureIDs);

    const char *textures[] = {
        "assets/texture.png",
        "assets/normal.png",
    };

    for (int i = 0; i < sizeof(s_textureIDs) / sizeof(s_textureIDs[0]); ++i) {
        if (loadTexture(s_textureIDs[i], textures[i]) != 0) {
            return -1;
        }
    }
    for (int i = 0; i < sizeof(s_textureIDs) / sizeof(s_textureIDs[0]); ++i) {
        s_textureHandles[i * 2] = glGetTextureHandleARB(s_textureIDs[i]);
        if (glGetError()) {
            std::cerr << "Failed to get texture handle" << std::endl;
            return -1;
        }
        glMakeTextureHandleResidentARB(s_textureHandles[i * 2]);
        if (glGetError()) {
            std::cerr << "Failed to get texture handle" << std::endl;
            return -1;
        }
    }
    glNamedBufferData(s_textureHandleBufferID, sizeof(s_textureHandles), s_textureHandles, GL_STATIC_DRAW);

    glGenVertexArrays(1, &s_allGeometryArrayID);
    s_allGeometryBuilder = new MeshBuilder(s_allGeometryArrayID);
    s_allGeometryBuilder->begin();

    GLuint mat;
    if (!Wavefront::loadObj(&s_models[0], mat, s_allGeometryBuilder, "model.obj")) {
        std::cerr << "Failed to load model" << std::endl;
        return -1;
    }
    s_allGeometryBuilder->commit();

    std::cout << "Loading complete" << std::endl;
    return 0;
}

int init(void) {
    //s_scene = new Scene(s_projectionMatrix);

    //s_player = new GameObject();
    //s_camera = new FPSCamera(s_player);

    //s_player->setPosition({ 0, 0, 0 });
    //s_camera->setRotation({ 0 * 3.14 / 180, -180 * 3.14 / 180, 0 });

    //s_scene->add(s_player);
    //s_scene->setActiveCamera(s_camera);

    // Setup drawcalls
    for (int i = -6; i <= 6; ++i) {
        for (int j = -6; j <= 6; ++j) {
            s_indirectCommands.push_back({ s_models[0].size, 1, s_models[0].begin, 0 });
            s_modelMatrices.push_back(glm::translate(glm::mat4(1), glm::vec3(i * 2, 0, j * 2)));
            s_meshAttribs.push_back({ 0 });
        }
    }

    glNamedBufferData(s_indirectCommandBufferID, s_indirectCommands.size() * sizeof(DrawArraysIndirectCommand), &s_indirectCommands[0], GL_STATIC_DRAW);
    glNamedBufferData(s_modelMatrixBufferID, s_modelMatrices.size() * sizeof(glm::mat4), &s_modelMatrices[0], GL_STATIC_DRAW);
    glNamedBufferData(s_meshAttribBufferID, s_meshAttribs.size() * sizeof(glm::mat4), &s_meshAttribs[0], GL_STATIC_DRAW);

    return 0;
}

int setup_gl(void) {
    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LESS);

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

    glGenBuffers(1, &s_modelMatrixBufferID);
    glGenBuffers(1, &s_indirectCommandBufferID);
    glGenBuffers(1, &s_meshAttribBufferID);
    glGenBuffers(1, &s_textureHandleBufferID);

    glBindBuffer(GL_DRAW_INDIRECT_BUFFER, s_indirectCommandBufferID);
    glBufferData(GL_DRAW_INDIRECT_BUFFER, s_indirectCommands.size() * sizeof(DrawArraysIndirectCommand), &s_indirectCommands[0], GL_STATIC_DRAW);
    glBindBuffer(GL_DRAW_INDIRECT_BUFFER, 0);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 1, s_modelMatrixBufferID);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 2, s_meshAttribBufferID);

    glBindBufferBase(GL_UNIFORM_BUFFER, 1, s_textureHandleBufferID);

    // Scene buffer

    glGenBuffers(1, &s_sceneUniformBufferID);

    glBindBuffer(GL_UNIFORM_BUFFER, s_sceneUniformBufferID);
    glBufferData(GL_UNIFORM_BUFFER, sizeof(s_sceneUniformData), &s_sceneUniformData, GL_DYNAMIC_DRAW);
    glBindBuffer(GL_UNIFORM_BUFFER, 0);

    glBindBufferBase(GL_UNIFORM_BUFFER, 0, s_sceneUniformBufferID);

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
    glBindFramebuffer(GL_FRAMEBUFFER, s_sceneBuffer);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
#endif

    // Move camera
    //double wt = t - s_walkStart;
    //double bobf = 0.05 * sin(wt * 10) * s_walk;
    //double ry = s_camera->getWorldRotation().y;
    //glm::vec3 delta(
        //s_moveSpeed * (sin(ry) * dt * s_walk - sin(ry + M_PI / 2) * dt * s_strafe),
        //s_moveSpeed * s_fly * dt,
        //-s_moveSpeed * (cos(ry) * dt * s_walk - cos(ry + M_PI / 2) * dt * s_strafe)
    //);

    //s_player->translate(delta);

    auto t0 = glfwGetTime();

    glBindVertexArray(s_allGeometryArrayID);
    glEnableVertexAttribArray(0);
    glEnableVertexAttribArray(1);
    glEnableVertexAttribArray(2);
    glEnableVertexAttribArray(3);
    glEnableVertexAttribArray(4);

    // BEGIN RENDER
    // BLAST 1
    glUseProgram(s_sceneShaderID);

    // UPDATE SCENE PARAMS HERE
    s_sceneUniformData.m_cameraPosition = glm::vec4(10, 10, 10, 1);
    s_sceneUniformData.m_cameraDestination = glm::vec4(0, 0, 0, 1);
    s_sceneUniformData.m_cameraMatrix = glm::lookAt(
        glm::vec3(s_sceneUniformData.m_cameraPosition),
        glm::vec3(s_sceneUniformData.m_cameraDestination),
        { 0, 1, 0 }
    );
    glNamedBufferSubData(s_sceneUniformBufferID, sizeof(glm::mat4), sizeof(SceneUniformData) - sizeof(glm::mat4),
        (GLvoid *) ((uintptr_t) &s_sceneUniformData + sizeof(glm::mat4)));
    //s_scene->render();

    glBindBuffer(GL_DRAW_INDIRECT_BUFFER, s_indirectCommandBufferID);

    auto t1 = glfwGetTime();
    glMultiDrawArraysIndirect(GL_TRIANGLES, 0, s_indirectCommands.size(), 0);
    auto t2 = glfwGetTime();

    s_transferTime += t1 - t0;
    s_drawCallTime += t2 - t1;
    // END RENDER

    glBindBuffer(GL_DRAW_INDIRECT_BUFFER, 0);

    glDisableVertexAttribArray(0);
    glDisableVertexAttribArray(1);
    glDisableVertexAttribArray(2);
    glDisableVertexAttribArray(3);
    glDisableVertexAttribArray(4);
    glBindVertexArray(0);

    glUseProgram(0);

    //Model::unbindAll();
    //Material::unbindAll();

#ifdef RENDER_TO_TEXTURE
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glViewport(0, 0, m_width, m_height);
    glUseProgram(s_screenShaderID);
    auto l = glGetUniformLocation(s_screenShaderID, "mDepthTexture");
    glUniform1i(l, 1);
    l = glGetUniformLocation(s_screenShaderID, "mTime");
    glUniform1f(l, (GLfloat) t);
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
    //s_scene->setViewport(m_width, m_height);
#endif

    s_sceneUniformData.m_projectionMatrix = glm::perspective(glm::radians(45.0f), ((float) width) / ((float) height), 0.1f, 100.0f);
    glNamedBufferSubData(s_sceneUniformBufferID, 0, sizeof(glm::mat4), &s_sceneUniformData);
}

void cursorPosCallback(GLFWwindow *win, double x, double y) {
    double cx = (x - m_width / 2) / m_width;
    double cy = (y - m_height / 2) / m_height;

    glfwSetCursorPos(win, m_width / 2, m_height / 2);

    //s_camera->rotate(glm::vec3(cy * 1.5, cx * 1.5, 0));
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

        //for (auto &obj: s_scene->meshObjects) {
            //obj->setRenderMode(s_renderMode);
        //}
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

    if (!GLEW_ARB_bindless_texture) {
        std::cerr << "Bindless textures are unsupported" << std::endl;
        return -2;
    }

    glfwSwapInterval(1);

    if (setup_gl() != 0 || loadData() != 0 || init() != 0) {
        glfwTerminate();
        return -1;
    }
    auto t0 = glfwGetTime();
    int frames = 0;

    while (!glfwWindowShouldClose(s_window)) {
        auto t1 = glfwGetTime();
        if (t1 - t0 > 1) {
            std::cout << frames << " frames" << std::endl;
            std::cout << s_frameTimeSum * 1000 << " ms" << std::endl;
            std::cout << (s_frameTimeSum / frames) * 1000 << " ms/frame" << std::endl;
            std::cout << (s_drawCallTime / frames) * 1000 << " ms/draw" << std::endl;
            std::cout << (s_transferTime / frames) * 1000 << " ms/xfer" << std::endl;
            t0 = t1;
            frames = 0;
            s_frameTimeSum = 0;
            s_drawCallTime = 0;
            s_transferTime = 0;
        }

        render();
        ++frames;

        glfwSwapBuffers(s_window);
        glfwPollEvents();
    }

    glfwTerminate();

    return 0;
}
