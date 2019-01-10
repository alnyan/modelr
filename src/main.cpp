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

#define R_SHADOW_MAP_WIDTH      4096
#define R_SHADOW_MAP_HEIGHT     4096
#define S_TEXTURE_COUNT         256
#define S_SHADOW_MAP_0          (S_TEXTURE_COUNT - 4)
#define textureIndex(i)         ((i) * 2)

static GLFWwindow *s_window;

static constexpr float s_moveSpeed = 5;
static int m_width, m_height;

struct DrawArraysIndirectCommand {
    GLuint count, instanceCount, first, baseInstance;
};

struct SceneUniformData {
    glm::mat4 m_projectionMatrix;
    glm::mat4 m_cameraMatrix;
    glm::vec4 m_cameraPosition;
    glm::vec4 m_cameraDestination;
};

struct SceneLight0UniformData {
    glm::mat4 m_projectionMatrix[4];
    glm::mat4 m_lightMatrix;
    glm::vec4 m_lightPosition;
};

struct MeshAttrib {
    int material;
};

// Dynamic objects
static GLuint s_sceneShaderID;
static GLuint s_allGeometryArrayID;
static GLuint s_modelMatrixBufferID,
              s_meshAttribBufferID,
              s_indirectCommandBufferID;
static GLuint s_textureHandleBufferID;
static GLuint s_textureIDs[2];
static GLuint64 s_textureHandles[S_TEXTURE_COUNT * 2];
static std::vector<DrawArraysIndirectCommand> s_indirectCommands;
static std::vector<glm::mat4> s_modelMatrices;
static std::vector<MeshAttrib> s_meshAttribs;
static MeshBuilder *s_allGeometryBuilder;
static Model s_models[3];

// Scene-global data
static GLuint s_sceneUniformBufferID;
static GLuint s_sceneLight0UniformBufferID;
static SceneUniformData s_sceneUniformData;
static float s_cameraPitch = 0, s_cameraYaw = 0;
static int s_walk = 0;
static int s_strafe = 0;
static int s_fly = 0;

// Time measurements
static double s_transferTime, s_drawCallTime;
static double s_meanFrameTime = 0;
static double s_frameTimeSum = 0;
static double s_lastTime = 0;

// Post-processing
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

// Lighting
static GLuint s_light0FramebufferID;
static GLuint s_light0DepthTextureIDs[4];
static GLuint s_depthShaderID;
static SceneLight0UniformData s_light0UniformData {
    {},
    {},
    glm::vec4(glm::normalize(glm::vec3(1, 1, 1)), 1)
};

static int s_viewType = 0;
static int s_shadowControl = 0;

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

    if (!Shader::loadProgram(s_screenShaderID, 2, GL_VERTEX_SHADER, "screen.vert", GL_FRAGMENT_SHADER, "screen.frag")) {
        std::cerr << "Failed to load screen shader" << std::endl;
        return -1;
    }

    if (!Shader::loadProgram(s_depthShaderID, 2, GL_VERTEX_SHADER, "depth.vert", GL_FRAGMENT_SHADER, "depth.frag")) {
        std::cerr << "Failed to load screen shader" << std::endl;
        return -1;
    }


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

int loadTextures(void) {
    const char *textures[] = {
        "assets/texture.png",
        "assets/normal.png",
    };

    glGenTextures(sizeof(textures) / sizeof(textures[0]), s_textureIDs);

    for (int i = 0; i < sizeof(textures) / sizeof(textures[0]); ++i) {
        if (loadTexture(s_textureIDs[i], textures[i]) != 0) {
            return -1;
        }
    }
    int e;
    for (int i = 0; i < 4; ++i) {
        std::cout << "Shadow map " << i << ": " << s_light0DepthTextureIDs[i] << std::endl;
        s_textureHandles[textureIndex(i + S_SHADOW_MAP_0)] = glGetTextureHandleARB(s_light0DepthTextureIDs[i]);
        if (glGetError()) {
            std::cerr << "Failed to get shadow map handle" << std::endl;
            return -1;
        }
        glMakeTextureHandleResidentARB(s_textureHandles[textureIndex(i + S_SHADOW_MAP_0)]);
        if (glGetError()) {
            std::cerr << "Failed to get shadow map handle" << std::endl;
            return -1;
        }
    }
    for (int i = 0; i < sizeof(textures) / sizeof(textures[0]); ++i) {
        s_textureHandles[textureIndex(i)] = glGetTextureHandleARB(s_textureIDs[i]);
        if ((e = glGetError())) {
            std::cerr << "Failed to get texture handle for " << i << std::endl;
            return -1;
        }
        glMakeTextureHandleResidentARB(s_textureHandles[textureIndex(i)]);
        if (glGetError()) {
            std::cerr << "Failed to make texture handle resident for " << i << std::endl;
            return -1;
        }
    }

    glGenBuffers(1, &s_textureHandleBufferID);

    glBindBuffer(GL_UNIFORM_BUFFER, s_textureHandleBufferID);
    glBufferData(GL_UNIFORM_BUFFER, sizeof(s_textureHandles), s_textureHandles, GL_STATIC_DRAW);
    glBindBuffer(GL_UNIFORM_BUFFER, 0);
    glBindBufferBase(GL_UNIFORM_BUFFER, 1, s_textureHandleBufferID);

    return 0;
}

int setup_gl(void) {
    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LESS);

    // Post-processing and render to texture setup
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

    // Light framebuffer setup
    glGenFramebuffers(1, &s_light0FramebufferID);
    glGenTextures(4, s_light0DepthTextureIDs);
    for (int i = 0; i < 4; ++i) {
        glBindTexture(GL_TEXTURE_2D, s_light0DepthTextureIDs[i]);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT24, R_SHADOW_MAP_WIDTH, R_SHADOW_MAP_HEIGHT, 0, GL_DEPTH_COMPONENT, GL_FLOAT, 0);
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

    // Main data setup
    glClearColor(0, 0.25, 0.25, 1);

    glGenBuffers(1, &s_modelMatrixBufferID);
    glGenBuffers(1, &s_indirectCommandBufferID);
    glGenBuffers(1, &s_meshAttribBufferID);

    glBindBuffer(GL_DRAW_INDIRECT_BUFFER, s_indirectCommandBufferID);
    glBufferData(GL_DRAW_INDIRECT_BUFFER, s_indirectCommands.size() * sizeof(DrawArraysIndirectCommand), &s_indirectCommands[0], GL_STATIC_DRAW);
    glBindBuffer(GL_DRAW_INDIRECT_BUFFER, 0);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 1, s_modelMatrixBufferID);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 2, s_meshAttribBufferID);


    // Scene buffer
    glGenBuffers(1, &s_sceneUniformBufferID);

    glBindBuffer(GL_UNIFORM_BUFFER, s_sceneUniformBufferID);
    glBufferData(GL_UNIFORM_BUFFER, sizeof(s_sceneUniformData), &s_sceneUniformData, GL_DYNAMIC_DRAW);
    glBindBuffer(GL_UNIFORM_BUFFER, 0);


    // Light0 buffer
    glGenBuffers(1, &s_sceneLight0UniformBufferID);
    glBindBuffer(GL_UNIFORM_BUFFER, s_sceneLight0UniformBufferID);
    glBufferData(GL_UNIFORM_BUFFER, sizeof(SceneLight0UniformData), &s_light0UniformData, GL_DYNAMIC_DRAW);
    glBindBuffer(GL_UNIFORM_BUFFER, 0);

    if (loadTextures()) {
        return -1;
    }

    glBindBufferBase(GL_UNIFORM_BUFFER, 0, s_sceneUniformBufferID);
    glBindBufferBase(GL_UNIFORM_BUFFER, 2, s_sceneLight0UniformBufferID);

    s_lastTime = glfwGetTime();

    return 0;
}

////

void updatePlayer(double t, double dt) {
    if (s_walk || s_strafe || s_fly) {
        auto walkDir = glm::normalize(glm::vec4(
            s_walk * cos(s_cameraYaw) + s_strafe * sin(s_cameraYaw),
            s_fly,
            s_walk * sin(s_cameraYaw) - s_strafe * cos(s_cameraYaw),
            0
        ));
        auto walkDelta = walkDir * (float) dt * 10.0f;

        s_sceneUniformData.m_cameraPosition += walkDelta;
        s_sceneUniformData.m_cameraDestination += walkDelta;
    }

    s_sceneUniformData.m_cameraMatrix = glm::lookAt(
        glm::vec3(s_sceneUniformData.m_cameraPosition),
        glm::vec3(s_sceneUniformData.m_cameraDestination),
        { 0, 1, 0 }
    );

    if (m_width && m_height) {
        float fov = 45.0f;
        float aspect = ((float) m_height) / ((float) m_width);
        s_sceneUniformData.m_projectionMatrix = glm::perspective(fov, 1 / aspect, 0.1f, 100.0f);
    }
}

void updateLight0(double t) {
    if (m_width != 0 && m_height != 0) {
        s_light0UniformData.m_lightMatrix = glm::lookAt(
            glm::vec3(s_light0UniformData.m_lightPosition),
            glm::vec3(0, 0, 0),
            glm::vec3(0, 1, 0)
        );

        auto lightP = s_light0UniformData.m_lightMatrix * glm::vec4(glm::vec3(s_sceneUniformData.m_cameraPosition), 1);
        float cascades[] {
            20,
            40,
            80,
            100
        };

        for (int i = 0; i < 4; ++i) {
            s_light0UniformData.m_projectionMatrix[i] = glm::ortho(
                lightP.x - cascades[i],
                lightP.x + cascades[i],
                lightP.y - cascades[i],
                lightP.y + cascades[i],
                -lightP.z - cascades[i],
                -lightP.z + cascades[i]
            );
        }
    }

}

void update(double t, double dt) {
    updatePlayer(t, dt);
    updateLight0(t);
}

////

void renderScene(void) {
    glBindFramebuffer(GL_FRAMEBUFFER, s_sceneBuffer);
    glViewport(0, 0, m_width, m_height);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glUseProgram(s_sceneShaderID);


    glMultiDrawArraysIndirect(GL_TRIANGLES, 0, s_indirectCommands.size(), 0);

    glBindTexture(GL_TEXTURE_2D, 0);
}

void renderLight0(int i) {
    glFramebufferTexture(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, s_light0DepthTextureIDs[i], 0);
    glViewport(0, 0, R_SHADOW_MAP_WIDTH, R_SHADOW_MAP_HEIGHT);
    glClear(GL_DEPTH_BUFFER_BIT);

    auto l = glGetUniformLocation(s_depthShaderID, "mRenderCascade");
    glUniform1i(l, i);

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
    auto dt = t - s_lastTime;
    s_lastTime = t;

    update(t, dt);

    glNamedBufferData(s_sceneUniformBufferID, sizeof(SceneUniformData), &s_sceneUniformData, GL_DYNAMIC_DRAW);
    glNamedBufferData(s_sceneLight0UniformBufferID, sizeof(SceneLight0UniformData), &s_light0UniformData, GL_DYNAMIC_DRAW);

    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glBindTexture(GL_TEXTURE_2D, 0);

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
    m_width = width;
    m_height = height;

    glBindTexture(GL_TEXTURE_2D, s_sceneTextures[0]);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB16F, m_width, m_height, 0, GL_RGB, GL_UNSIGNED_BYTE, 0);
    glBindTexture(GL_TEXTURE_2D, s_sceneTextures[1]);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT24, m_width, m_height, 0, GL_DEPTH_COMPONENT, GL_FLOAT, 0);
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
    }
    if (key == GLFW_KEY_S) {
        s_walk = -!!action;
    }
    if (key == GLFW_KEY_A) {
        s_strafe = !!action;
    }
    if (key == GLFW_KEY_D) {
        s_strafe = -!!action;
    }
    if (key == GLFW_KEY_SPACE) {
        s_fly = !!action;
    }
    if (key == GLFW_KEY_LEFT_SHIFT) {
        s_fly = -!!action;
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

    if (loadData() != 0 || setup_gl() != 0 || init() != 0) {
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
