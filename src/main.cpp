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

//#define R_FRUSTUM_LENGTH 8

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

static GLuint s_light0FramebufferID;
static GLuint s_light0DepthTextureIDs[4];
static GLuint s_depthShaderID;
static glm::mat4 s_light0ProjectionMatrices[4];
static glm::vec3 s_light0Position = glm::vec3(1, 1, 0);
static glm::mat4 s_cameraProjectionMatrix;

static int s_viewType = 0;
static int s_shadowControl = 0;
static float s_cameraPitch = 0, s_cameraYaw = 0;

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

    if (!Shader::loadProgram(s_depthShaderID, 2, GL_VERTEX_SHADER, "depth.vert", GL_FRAGMENT_SHADER, "depth.frag")) {
        std::cerr << "Failed to load screen shader" << std::endl;
        return -1;
    }
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
    if (!Wavefront::loadObj(&s_models[1], mat, s_allGeometryBuilder, "terrain.obj")) {
        std::cerr << "Failed to load model" << std::endl;
        return -1;
    }
    s_allGeometryBuilder->commit();

    std::cout << "Loading complete" << std::endl;
    return 0;
}

int init(void) {
    s_sceneUniformData.m_cameraPosition = glm::vec4(2, 2, 2, 1);
    s_sceneUniformData.m_cameraDestination = glm::vec4(2, 2, 3, 1);

    // Setup drawcalls
    s_indirectCommands.push_back({ s_models[1].size, 1, s_models[1].begin, 0 });
    s_modelMatrices.push_back(glm::mat4(1));
    s_meshAttribs.push_back({ 0 });

    for (int i = -24; i <= 6; ++i) {
        for (int j = -24; j <= 6; ++j) {
            s_indirectCommands.push_back({ s_models[0].size, 1, s_models[0].begin, 0 });
            s_modelMatrices.push_back(glm::translate(glm::mat4(1), glm::vec3(i * 2, 0, j * 2)));
            s_meshAttribs.push_back({ 0 });
        }
    }
    for (int i = -5; i <= 5; ++i) {
        for (int j = 1; j <= 3; ++j) {
            s_indirectCommands.push_back({ s_models[0].size, 1, s_models[0].begin, 0 });
            s_modelMatrices.push_back(glm::translate(glm::mat4(1), glm::vec3(i * 2, j * 2, 4)));
            s_meshAttribs.push_back({ 0 });
        }
    }
    for (int i = -5; i <= 5; ++i) {
        for (int j = 1; j <= 3; ++j) {
            s_indirectCommands.push_back({ s_models[0].size, 1, s_models[0].begin, 0 });
            s_modelMatrices.push_back(glm::translate(glm::mat4(1), glm::vec3(i * 2, j * 2, -4)));
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

#ifdef RENDER_TO_TEXTURE
    glGenFramebuffers(1, &s_sceneBuffer);
    glGenTextures(2, s_sceneTextures);
    glGenRenderbuffers(1, &s_depthBufferID);

    glBindFramebuffer(GL_FRAMEBUFFER, s_sceneBuffer);

    glBindTexture(GL_TEXTURE_2D, s_sceneTextures[0]);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB16F, 800, 600, 0, GL_RGB, GL_UNSIGNED_BYTE, 0);
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
    glGenFramebuffers(1, &s_light0FramebufferID);
    glGenTextures(4, s_light0DepthTextureIDs);

    for (int i = 0; i < 4; ++i) {
        glBindTexture(GL_TEXTURE_2D, s_light0DepthTextureIDs[i]);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT24, 800, 600, 0, GL_DEPTH_COMPONENT, GL_FLOAT, 0);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
    }
    glBindTexture(GL_TEXTURE_2D, 0);

    glBindFramebuffer(GL_FRAMEBUFFER, s_light0FramebufferID);
    glFramebufferTexture(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, s_light0DepthTextureIDs[0], 0);
    glDrawBuffer(GL_NONE);

    if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
        printf("0x%x\n", glCheckFramebufferStatus(GL_FRAMEBUFFER));
        std::cerr << "Failed to create framebuffer" << std::endl;
        return -1;
    }
    glBindFramebuffer(GL_FRAMEBUFFER, 0);

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

void renderScene(void) {
    glBindFramebuffer(GL_FRAMEBUFFER, s_sceneBuffer);
    glViewport(0, 0, m_width, m_height);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glUseProgram(s_sceneShaderID);

    {
        auto l = glGetUniformLocation(s_sceneShaderID, "mDepth0");
        auto depthMVP = s_light0ProjectionMatrices[0] * s_sceneUniformData.m_cameraMatrix;
        glUniformMatrix4fv(l, 1, GL_FALSE, &depthMVP[0][0]);
        l = glGetUniformLocation(s_sceneShaderID, "mDepth1");
        depthMVP = s_light0ProjectionMatrices[1] * s_sceneUniformData.m_cameraMatrix;
        glUniformMatrix4fv(l, 1, GL_FALSE, &depthMVP[0][0]);
        l = glGetUniformLocation(s_sceneShaderID, "mDepth2");
        depthMVP = s_light0ProjectionMatrices[2] * s_sceneUniformData.m_cameraMatrix;
        glUniformMatrix4fv(l, 1, GL_FALSE, &depthMVP[0][0]);
        l = glGetUniformLocation(s_sceneShaderID, "mDepth3");
        depthMVP = s_light0ProjectionMatrices[3] * s_sceneUniformData.m_cameraMatrix;
        glUniformMatrix4fv(l, 1, GL_FALSE, &depthMVP[0][0]);
        l = glGetUniformLocation(s_sceneShaderID, "mShadowMap1");
        glUniform1i(l, 1);
        l = glGetUniformLocation(s_sceneShaderID, "mShadowMap2");
        glUniform1i(l, 2);
        l = glGetUniformLocation(s_sceneShaderID, "mShadowMap3");
        glUniform1i(l, 3);
        glBindTextures(0, 4, s_light0DepthTextureIDs);
    }

    s_sceneUniformData.m_cameraMatrix = glm::lookAt(
        glm::vec3(s_sceneUniformData.m_cameraPosition),
        glm::vec3(s_sceneUniformData.m_cameraDestination),
        glm::vec3(0, 1, 0)
    );
    s_sceneUniformData.m_projectionMatrix = s_cameraProjectionMatrix;

    glNamedBufferSubData(s_sceneUniformBufferID, 0, sizeof(SceneUniformData), &s_sceneUniformData);

    glMultiDrawArraysIndirect(GL_TRIANGLES, 0, s_indirectCommands.size(), 0);

    glBindTexture(GL_TEXTURE_2D, 0);
}

void renderLight0(int i) {
    glFramebufferTexture(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, s_light0DepthTextureIDs[i], 0);
    glViewport(0, 0, m_width, m_height);
    glClear(GL_DEPTH_BUFFER_BIT);

    s_sceneUniformData.m_cameraMatrix = glm::lookAt(
        s_light0Position,
        glm::vec3(0, 0, 0),
        glm::vec3(0, 1, 0)
    );
    s_sceneUniformData.m_projectionMatrix = s_light0ProjectionMatrices[i];
    glNamedBufferSubData(s_sceneUniformBufferID, 0, sizeof(SceneUniformData), &s_sceneUniformData);

    glMultiDrawArraysIndirect(GL_TRIANGLES, 0, s_indirectCommands.size(), 0);
}

void renderScreen(void) {
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glViewport(0, 0, m_width, m_height);
    glUseProgram(s_screenShaderID);
    auto l = glGetUniformLocation(s_screenShaderID, "mDepthTexture");
    glUniform1i(l, 1);
    l = glGetUniformLocation(s_screenShaderID, "mScreenDimensions");
    glm::vec4 screenDimensions(m_width, m_height, 0.1f, 100.0f);
    glUniform4f(l, screenDimensions.x, screenDimensions.y, screenDimensions.z, screenDimensions.w);

    glBindTextures(0, 2, s_sceneTextures);
    if (s_viewType)
        glBindTexture(GL_TEXTURE_2D, s_light0DepthTextureIDs[s_viewType - 1]);

    glBindVertexArray(s_screenArrayID);
    glEnableVertexAttribArray(0);
    glEnableVertexAttribArray(1);
    glDrawArrays(GL_TRIANGLES, 0, 6);
    glDisableVertexAttribArray(0);
    glDisableVertexAttribArray(1);
    glBindVertexArray(0);
}

void render(void) {
    auto t = glfwGetTime();
    s_lastTime = t;

    s_light0Position = glm::normalize(glm::vec3(1, 1, 1));

    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glBindTexture(GL_TEXTURE_2D, 0);

    if (m_width != 0 && m_height != 0) {
        if (s_walk || s_strafe || s_fly) {
            auto walkDir = glm::normalize(glm::vec4(
                s_walk * cos(s_cameraYaw) + s_strafe * sin(s_cameraYaw),
                s_fly,
                s_walk * sin(s_cameraYaw) - s_strafe * cos(s_cameraYaw),
                0
            ));
            auto walkDelta = walkDir * 0.1f;


            s_sceneUniformData.m_cameraPosition += walkDelta;
            s_sceneUniformData.m_cameraDestination += walkDelta;
        }
        float fov = 45.0f;
        float aspect = ((float) m_height) / ((float) m_width);
        s_cameraProjectionMatrix = glm::perspective(fov, 1 / aspect, 0.1f, 100.0f);

        auto lightMatrix = glm::lookAt(
            s_light0Position,
            glm::vec3(0, 0, 0),
            glm::vec3(0, 1, 0)
        );
        auto lightP = lightMatrix * glm::vec4(glm::vec3(s_sceneUniformData.m_cameraPosition), 1);
        float cascades[] {
            20,
            40,
            80,
            100
        };

        for (int i = 0; i < 4; ++i) {
            s_light0ProjectionMatrices[i] = glm::ortho(
                lightP.x - cascades[i],
                lightP.x + cascades[i],
                lightP.y - cascades[i],
                lightP.y + cascades[i],
                -lightP.z - cascades[i],
                -lightP.z + cascades[i]
            );
        }
    }

    glBindVertexArray(s_allGeometryArrayID);
    glBindBuffer(GL_DRAW_INDIRECT_BUFFER, s_indirectCommandBufferID);
    glEnableVertexAttribArray(0);
    glEnableVertexAttribArray(1);
    glEnableVertexAttribArray(2);
    glEnableVertexAttribArray(3);
    glEnableVertexAttribArray(4);

    // BEGIN RENDER
    // BLAST 1: LIGHT'S ORTHO
    glUseProgram(s_depthShaderID);
    glCullFace(GL_FRONT);
    glBindFramebuffer(GL_FRAMEBUFFER, s_light0FramebufferID);
    for (int i = 0; i < 4; ++i) {
        renderLight0(i);
    }
    glCullFace(GL_BACK);

    // BLAST 2: PLAYER'S CAMERA
    renderScene();

    // END RENDER

    glBindBuffer(GL_DRAW_INDIRECT_BUFFER, 0);
    glBindVertexArray(0);

    glDisableVertexAttribArray(0);
    glDisableVertexAttribArray(1);
    glDisableVertexAttribArray(2);
    glDisableVertexAttribArray(3);
    glDisableVertexAttribArray(4);

    glUseProgram(0);

    renderScreen();

    auto r = glfwGetTime();

    s_frameTimeSum += r - t;
}

//

void windowSizeCallback(GLFWwindow *win, int width, int height) {
    glViewport(0, 0, width, height);
    m_width = width;
    m_height = height;

    glBindTexture(GL_TEXTURE_2D, s_sceneTextures[0]);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB16F, m_width, m_height, 0, GL_RGB, GL_UNSIGNED_BYTE, 0);
    glBindTexture(GL_TEXTURE_2D, s_sceneTextures[1]);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT24, m_width, m_height, 0, GL_DEPTH_COMPONENT, GL_FLOAT, 0);
    for (int i = 0; i < 4; ++i) {
        glBindTexture(GL_TEXTURE_2D, s_light0DepthTextureIDs[i]);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT24, m_width, m_height, 0, GL_DEPTH_COMPONENT, GL_FLOAT, 0);
    }
    glBindTexture(GL_TEXTURE_2D, 0);

}

void cursorPosCallback(GLFWwindow *win, double x, double y) {
    double cx = (x - m_width / 2) / m_width;
    double cy = (y - m_height / 2) / m_height;

    if (m_width == 0 || m_height == 0) {
        return;
    }

    glfwSetCursorPos(win, m_width / 2, m_height / 2);

    s_cameraPitch -= cy;
    s_cameraYaw += cx;

    //glm::mat4 matPitch(1), matYaw(1);
    //matPitch = glm::rotate(matPitch, s_cameraPitch, glm::vec3(1, 0, 0));
    //matYaw = glm::rotate(matYaw, s_cameraYaw, glm::vec3(0, 1, 0));
    //auto rot = matPitch * matYaw;
    //camVector = rot * camVector;

    s_sceneUniformData.m_cameraDestination = s_sceneUniformData.m_cameraPosition + glm::vec4(cos(s_cameraYaw), 0, glm::sin(s_cameraYaw), 0);
}

void keyCallback(GLFWwindow *win, int key, int scan, int action, int mods) {
    if (key == GLFW_KEY_T && action == 1) {
        ++s_viewType;
        s_viewType %= 5;
    }
    if (key == GLFW_KEY_EQUAL && action == 1) {
        ++s_shadowControl;
    }
    if (key == GLFW_KEY_MINUS && action == 1) {
        --s_shadowControl;
    }

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
