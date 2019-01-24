#include "renderer.h"
#include <iostream>
#include <math.h>
#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include "render/common.h"
#include "render/shader.h"
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include "gameobject.h"
#include <algorithm>
#include <list>
#include <map>

#include "render/wavefront.h"
#include "gldynamicarray.h"
#include "resources.h"
#include "modelobject.h"
#include "input.h"

#include "config.h"

#define WINDOW_WIDTH    1920
#define WINDOW_HEIGHT   1080


static GLFWwindow *s_window;

int Renderer::width, Renderer::height;

// Particles
static GLuint s_particleDataBufferID;
static Particle *s_particles = nullptr;
static GLuint s_particleGeometryArrayID;
static GLuint s_sceneBillboardShaderID;

static GLuint s_sceneShaderID;
static GLuint s_allGeometryArrayID;

static MeshBuilder *s_allGeometryBuilder;

// Resources
static GLuint s_textureHandleBufferID,
              s_materialBufferID;

// Scene-global data
static GLuint s_sceneUniformBufferID;
static GLuint s_sceneLight0UniformBufferID;
static Camera s_camera;
Camera *Renderer::camera = &s_camera;
static Scene s_scene;
Scene *Renderer::scene = &s_scene;

// Time measurements
static double s_transferTime, s_drawCallTime;
static double s_meanFrameTime = 0;
static double s_frameTimeSum = 0;
static double s_lastTime = 0;
static int s_lastParticle = 0;
static double s_lastObjectTime = 0;
static bool s_phys = true;

// Post-processing
static GLuint s_sceneBuffer;
static GLuint s_sceneTextures[3];
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
static GLuint s_light0DepthTextureIDs[S_SHADOW_CASCADES];
static GLuint s_depthShaderID, s_billboardDepthShaderID;
static SceneLight0UniformData s_light0UniformData {
    {},
    {},
    glm::vec4(glm::normalize(glm::vec3(1, 1, 1)), 1)
};

// Particles
static float s_lastParticleTime;

static int s_viewType = 0;
static int s_renderType = 0;
static int s_shadowControl = 0;


static void windowSizeCallback(GLFWwindow *win, int width, int height) {
    Renderer::width = width;
    Renderer::height = height;

    glBindTexture(GL_TEXTURE_2D, s_sceneTextures[0]);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB16F, Renderer::width, Renderer::height, 0, GL_RGB, GL_FLOAT, 0);
    glBindTexture(GL_TEXTURE_2D, s_sceneTextures[1]);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT24, Renderer::width, Renderer::height, 0, GL_DEPTH_COMPONENT, GL_FLOAT, 0);
    glBindTexture(GL_TEXTURE_2D, s_sceneTextures[2]);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB16F, Renderer::width, Renderer::height, 0, GL_RGB, GL_FLOAT, 0);
    glBindTexture(GL_TEXTURE_2D, 0);
}

int Renderer::loadData() {
    std::cout << "Loading data" << std::endl;

    // Prepare shader defines
    Shader::addDefinition("S_ATTRIB_VERTEX", S_ATTRIB_VERTEX);
    Shader::addDefinition("S_ATTRIB_TEXCOORD", S_ATTRIB_TEXCOORD);
    Shader::addDefinition("S_ATTRIB_NORMAL", S_ATTRIB_NORMAL);
    Shader::addDefinition("S_ATTRIB_TANGENT", S_ATTRIB_TANGENT);
    Shader::addDefinition("S_ATTRIB_BITANGENT", S_ATTRIB_BITANGENT);
    Shader::addDefinition("S_PARTICLE_ATTRIB_VERTEX", S_PARTICLE_ATTRIB_VERTEX);
    Shader::addDefinition("S_PARTICLE_ATTRIB_TEXCOORD", S_PARTICLE_ATTRIB_TEXCOORD);
    Shader::addDefinition("S_UBO_SCENE", S_UBO_SCENE);
    Shader::addDefinition("S_UBO_TEXTURES", S_UBO_TEXTURES);
    Shader::addDefinition("S_UBO_LIGHT0", S_UBO_LIGHT0);
    Shader::addDefinition("S_UBO_MATERIALS", S_UBO_MATERIALS);
    Shader::addDefinition("S_SSBO_MODEL", S_SSBO_MODEL);
    Shader::addDefinition("S_SSBO_MESH_ATTRIB", S_SSBO_MESH_ATTRIB);
    Shader::addDefinition("S_SSBO_PARTICLE", S_SSBO_PARTICLE);
    Shader::addDefinition("S_TEXTURE_COUNT", S_TEXTURE_COUNT);
    Shader::addDefinition("S_SHADOW_CASCADES", S_SHADOW_CASCADES);
    Shader::addDefinition("S_SHADOW_MAP_0", S_SHADOW_MAP_0);
    Shader::addDefinition("S_TEXTURE_UNDEFINED", S_TEXTURE_UNDEFINED);
    Shader::addDefinition("S_MATERIAL_COUNT", S_MATERIAL_COUNT);

    //

    if (!Shader::loadProgram(s_sceneShaderID, 2, GL_VERTEX_SHADER, "shader.vert", GL_FRAGMENT_SHADER, "shader.frag")) {
        std::cerr << "Failed to load shaders" << std::endl;
        return -1;
    }

    if (!Shader::loadProgram(s_sceneBillboardShaderID, 2, GL_VERTEX_SHADER, "billboard.vert", GL_FRAGMENT_SHADER, "billboard.frag")) {
        std::cerr << "Failed to load shaders" << std::endl;
        return -1;
    }

    if (!Shader::loadProgram(s_billboardDepthShaderID, 2, GL_VERTEX_SHADER, "billboard-depth.vert", GL_FRAGMENT_SHADER, "depth.frag")) {
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

    loadTexture("assets/smoke.png");

    glGenVertexArrays(1, &s_allGeometryArrayID);
    s_allGeometryBuilder = new MeshBuilder(s_allGeometryArrayID);
    s_allGeometryBuilder->begin();

    if (loadModel(s_allGeometryBuilder, "model.obj")) {
        std::cout << "Failed to load models" << std::endl;
        return -1;
    }
    if (loadModel(s_allGeometryBuilder, "terrain.obj")) {
        std::cout << "Failed to load models" << std::endl;
        return -1;
    }
    if (loadModel(s_allGeometryBuilder, "multipart.obj")) {
        std::cout << "Failed to load models" << std::endl;
        return -1;
    }
    if (loadModel(s_allGeometryBuilder, "garage.obj")) {
        std::cout << "Failed to load models" << std::endl;
        return -1;
    }

    s_allGeometryBuilder->commit();

    glGenBuffers(1, &s_materialBufferID);
    uploadMaterials(s_materialBufferID);
    glBindBufferBase(GL_UNIFORM_BUFFER, S_UBO_MATERIALS, s_materialBufferID);

    std::cout << "Loading complete" << std::endl;
    return 0;
}

int Renderer::init_gl() {
    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LESS);

    // Post-processing and render to texture setup
    glGenFramebuffers(1, &s_sceneBuffer);
    glGenTextures(4, s_sceneTextures);
    glGenRenderbuffers(1, &s_depthBufferID);

    glBindFramebuffer(GL_FRAMEBUFFER, s_sceneBuffer);

    glBindTexture(GL_TEXTURE_2D, s_sceneTextures[0]);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB16F, WINDOW_WIDTH, WINDOW_HEIGHT, 0, GL_RGB, GL_UNSIGNED_BYTE, 0);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glBindTexture(GL_TEXTURE_2D, s_sceneTextures[1]);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT24, WINDOW_WIDTH, WINDOW_HEIGHT, 0, GL_DEPTH_COMPONENT, GL_FLOAT, 0);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glBindTexture(GL_TEXTURE_2D, s_sceneTextures[2]);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB16F, WINDOW_WIDTH, WINDOW_HEIGHT, 0, GL_RGB, GL_FLOAT, 0);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glBindTexture(GL_TEXTURE_2D, 0);

    glFramebufferTexture(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, s_sceneTextures[0], 0);
    glFramebufferTexture(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT1, s_sceneTextures[2], 0);
    glFramebufferTexture(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, s_sceneTextures[1], 0);
    GLenum db[] { GL_COLOR_ATTACHMENT0, GL_COLOR_ATTACHMENT1 };
    glDrawBuffers(2, db);

    glGenVertexArrays(1, &s_screenArrayID);
    glGenBuffers(1, &s_screenBufferID);

    glBindVertexArray(s_screenArrayID);
    glBindBuffer(GL_ARRAY_BUFFER, s_screenBufferID);
    glBufferData(GL_ARRAY_BUFFER, sizeof(s_screenQuadData), s_screenQuadData, GL_STATIC_DRAW);
    glVertexAttribPointer(S_ATTRIB_VERTEX, 3, GL_FLOAT, GL_FALSE, sizeof(GLfloat) * 5, 0);
    glVertexAttribPointer(S_ATTRIB_TEXCOORD, 2, GL_FLOAT, GL_FALSE, sizeof(GLfloat) * 5, (GLvoid *) (sizeof(GLfloat) * 3));
    glBindVertexArray(0);

    if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
        std::cerr << "Failed to create framebuffer" << std::endl;
        return -1;
    }

    // Light framebuffer setup
    glGenFramebuffers(1, &s_light0FramebufferID);
    glGenTextures(S_SHADOW_CASCADES, s_light0DepthTextureIDs);
    for (int i = 0; i < S_SHADOW_CASCADES; ++i) {
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

    // Scene buffer
    glGenBuffers(1, &s_sceneUniformBufferID);

    glBindBuffer(GL_UNIFORM_BUFFER, s_sceneUniformBufferID);
    glBufferData(GL_UNIFORM_BUFFER, sizeof(&s_camera.uniformData), &s_camera.uniformData, GL_DYNAMIC_DRAW);
    glBindBuffer(GL_UNIFORM_BUFFER, 0);

    // Light0 buffer
    glGenBuffers(1, &s_sceneLight0UniformBufferID);
    glBindBuffer(GL_UNIFORM_BUFFER, s_sceneLight0UniformBufferID);
    glBufferData(GL_UNIFORM_BUFFER, sizeof(SceneLight0UniformData), &s_light0UniformData, GL_DYNAMIC_DRAW);
    glBindBuffer(GL_UNIFORM_BUFFER, 0);

    if (loadTextures()) {
        return -1;
    }

    glBindBufferBase(GL_UNIFORM_BUFFER, S_UBO_SCENE, s_sceneUniformBufferID);
    glBindBufferBase(GL_UNIFORM_BUFFER, S_UBO_LIGHT0, s_sceneLight0UniformBufferID);

    // Particle buffers
    GLuint particleGeometryBuffer;
    glGenVertexArrays(1, &s_particleGeometryArrayID);
    glGenBuffers(1, &particleGeometryBuffer);

    glBindVertexArray(s_particleGeometryArrayID);
    glBindBuffer(GL_ARRAY_BUFFER, particleGeometryBuffer);
    glBufferData(GL_ARRAY_BUFFER, sizeof(s_screenQuadData), s_screenQuadData, GL_STATIC_DRAW);
    glVertexAttribPointer(S_PARTICLE_ATTRIB_VERTEX, 3, GL_FLOAT, GL_FALSE, sizeof(GLfloat) * 5, 0);
    glVertexAttribPointer(S_PARTICLE_ATTRIB_TEXCOORD, 2, GL_FLOAT, GL_FALSE, sizeof(GLfloat) * 5, (GLvoid *) (3 * sizeof(GLfloat)));
    glBindVertexArray(0);

    assert(sizeof(Particle) == sizeof(glm::mat4));
    glGenBuffers(1, &s_particleDataBufferID);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, S_SSBO_PARTICLE, s_particleDataBufferID);
    glNamedBufferStorage(s_particleDataBufferID, R_PARTICLE_MAX * sizeof(Particle), NULL, GL_MAP_WRITE_BIT | GL_MAP_PERSISTENT_BIT | GL_MAP_COHERENT_BIT);

    if (!(s_particles = (Particle *) glMapNamedBufferRange(s_particleDataBufferID, 0, R_PARTICLE_MAX * sizeof(Particle), GL_MAP_WRITE_BIT | GL_MAP_PERSISTENT_BIT | GL_MAP_COHERENT_BIT))) {
        std::cerr << "Failed to generate particle buffer" << std::endl;
        return -1;
    }

    s_lastTime = glfwGetTime();

    return 0;
}

int Renderer::loadTextures() {
    int e;

    for (int i = 0; i < S_SHADOW_CASCADES; ++i) {
        std::cout << "Shadow map " << i << ": " << s_light0DepthTextureIDs[i] << std::endl;

        if (genTextureHandle(i + S_SHADOW_MAP_0, s_light0DepthTextureIDs[i]) != 0) {
            return -1;
        }
    }

    glGenBuffers(1, &s_textureHandleBufferID);

    return 0;
}

////

void updateLight0(double t) {
    if (Renderer::width != 0 && Renderer::height != 0) {
        s_light0UniformData.m_lightMatrix = glm::lookAt(
            glm::vec3(s_light0UniformData.m_lightPosition),
            glm::vec3(0, 0, 0),
            glm::vec3(0, 1, 0)
        );

        auto lightP = s_light0UniformData.m_lightMatrix * glm::vec4(glm::vec3(s_camera.uniformData.m_cameraPosition), 1);
        float cascades[S_SHADOW_CASCADES] {
            20,
            40,
            80,
            100
        };

        for (int i = 0; i < S_SHADOW_CASCADES; ++i) {
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

////

void renderScene(void) {
    auto t0 = glfwGetTime();
    glUseProgram(s_sceneShaderID);
    auto t1 = glfwGetTime();

    // Draw static objects
    s_scene.staticModelObjectBuffers.bind();
    glMultiDrawArraysIndirect(s_renderType ? GL_LINES : GL_TRIANGLES, 0, s_scene.staticModelObjectBuffers.getSize(), 0);
    // Draw dynamic objects
    s_scene.dynamicModelObjectBuffers.bind();
    glMultiDrawArraysIndirect(s_renderType ? GL_LINES : GL_TRIANGLES, 0, s_scene.dynamicModelObjectBuffers.getSize(), 0);

    s_drawCallTime += glfwGetTime() - t1;
    s_transferTime += t1 - t0;
}

void renderLight0(int i) {
    auto t0 = glfwGetTime();
    glFramebufferTexture(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, s_light0DepthTextureIDs[i], 0);
    glViewport(0, 0, R_SHADOW_MAP_WIDTH, R_SHADOW_MAP_HEIGHT);
    glClear(GL_DEPTH_BUFFER_BIT);
    glUseProgram(s_depthShaderID);

    auto l = glGetUniformLocation(s_depthShaderID, "mRenderCascade");
    glUniform1i(l, i);

    auto t1 = glfwGetTime();
    // Draw static objects
    s_scene.staticModelObjectBuffers.bind();
    glMultiDrawArraysIndirect(s_renderType ? GL_LINES : GL_TRIANGLES, 0, s_scene.staticModelObjectBuffers.getSize(), 0);
    // Draw dynamic objects
    s_scene.dynamicModelObjectBuffers.bind();
    glMultiDrawArraysIndirect(s_renderType ? GL_LINES : GL_TRIANGLES, 0, s_scene.dynamicModelObjectBuffers.getSize(), 0);

    s_drawCallTime += glfwGetTime() - t1;
    s_transferTime += t1 - t0;
}

void renderScreen(void) {
    auto t0 = glfwGetTime();
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glViewport(0, 0, Renderer::width, Renderer::height);
    glUseProgram(s_screenShaderID);
    auto l = glGetUniformLocation(s_screenShaderID, "mDepthTexture");
    glUniform1i(l, 1);
    l = glGetUniformLocation(s_screenShaderID, "mVelocityTexture");
    glUniform1i(l, 2);
    l = glGetUniformLocation(s_screenShaderID, "mScreenDimensions");
    glm::vec4 screenDimensions(Renderer::width, Renderer::height, 0.1f, 100.0f);
    glUniform4f(l, screenDimensions.x, screenDimensions.y, screenDimensions.z, screenDimensions.w);

    glBindTextures(0, 3, s_sceneTextures);
    if (s_viewType && s_viewType < 5)
        glBindTexture(GL_TEXTURE_2D, s_light0DepthTextureIDs[s_viewType - 1]);
    else if (s_viewType >= 5)
        glBindTexture(GL_TEXTURE_2D, s_sceneTextures[s_viewType - 5 + 2]);

    glBindVertexArray(s_screenArrayID);
    glEnableVertexAttribArray(S_ATTRIB_VERTEX);
    glEnableVertexAttribArray(S_ATTRIB_TEXCOORD);
    auto t1 = glfwGetTime();
    glDrawArrays(GL_TRIANGLES, 0, 6);
    glDisableVertexAttribArray(S_ATTRIB_TEXCOORD);
    glDisableVertexAttribArray(S_ATTRIB_VERTEX);
    glBindVertexArray(0);

    s_drawCallTime += glfwGetTime() - t1;
    s_transferTime += t1 - t0;
}

void Renderer::render() {
    auto t = glfwGetTime();
    auto dt = t - s_lastTime;
    s_lastTime = t;

    updateLight0(t);

    auto t0 = glfwGetTime();
    glNamedBufferData(s_sceneUniformBufferID, sizeof(SceneUniformData), &s_camera.uniformData, GL_DYNAMIC_DRAW);
    glNamedBufferData(s_sceneLight0UniformBufferID, sizeof(SceneLight0UniformData), &s_light0UniformData, GL_DYNAMIC_DRAW);

    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glBindTexture(GL_TEXTURE_2D, 0);

    // BEGIN RENDER
    // BLAST 1: LIGHT'S ORTHO
    glBindVertexArray(s_allGeometryArrayID);
    glEnableVertexAttribArray(S_ATTRIB_VERTEX);
    glEnableVertexAttribArray(S_ATTRIB_TEXCOORD);
    glEnableVertexAttribArray(S_ATTRIB_NORMAL);
    glEnableVertexAttribArray(S_ATTRIB_TANGENT);
    glEnableVertexAttribArray(S_ATTRIB_BITANGENT);

    glCullFace(GL_FRONT);
    glBindFramebuffer(GL_FRAMEBUFFER, s_light0FramebufferID);
    auto t1 = glfwGetTime();
    for (int i = 0; i < S_SHADOW_CASCADES; ++i) {
        renderLight0(i);
    }
    auto t2 = glfwGetTime();
    glCullFace(GL_BACK);

    // BLAST 2: PLAYER'S CAMERA
    glBindFramebuffer(GL_FRAMEBUFFER, s_sceneBuffer);
    glViewport(0, 0, Renderer::width, Renderer::height);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    auto t3 = glfwGetTime();
    renderScene();

    auto t4 = glfwGetTime();
    // BLAST 3: PARTICLES
    glBindBuffer(GL_DRAW_INDIRECT_BUFFER, 0);
    glDisableVertexAttribArray(S_ATTRIB_BITANGENT);
    glDisableVertexAttribArray(S_ATTRIB_TANGENT);
    glDisableVertexAttribArray(S_ATTRIB_NORMAL);
    glDisableVertexAttribArray(S_ATTRIB_TEXCOORD);
    glDisableVertexAttribArray(S_ATTRIB_VERTEX);

    //glBindVertexArray(s_particleGeometryArrayID);
    //glEnableVertexAttribArray(S_PARTICLE_ATTRIB_VERTEX);
    //glEnableVertexAttribArray(S_PARTICLE_ATTRIB_TEXCOORD);
    //glUseProgram(s_sceneBillboardShaderID);
    //glEnable(GL_BLEND);
    //////glBlendFunc(GL_SRC_ALPHA, GL_ONE);
    //glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    //glDepthMask(GL_FALSE);
    //auto l = glGetUniformLocation(s_sceneBillboardShaderID, "mTime");
    //glUniform1f(l, t);
    auto t5 = glfwGetTime();
    //glDrawArraysInstancedBaseInstance(GL_TRIANGLES, 0, 6, R_PARTICLE_MAX, 0);
    auto t6 = glfwGetTime();
    //glDepthMask(GL_TRUE);
    //glDisable(GL_BLEND);

    glBindVertexArray(0);
    //glDisableVertexAttribArray(S_PARTICLE_ATTRIB_VERTEX);
    //glDisableVertexAttribArray(S_PARTICLE_ATTRIB_TEXCOORD);

    // END RENDER
    glUseProgram(0);

    auto t7 = glfwGetTime();
    renderScreen();

    auto r = glfwGetTime();

    s_frameTimeSum += r - t;
    s_transferTime += (t7 - t6) + (t5 - t4) + (t3 - t2) + (t1 - t0);

    glfwSwapBuffers(s_window);
}

void Renderer::pollEvents() {
    glfwPollEvents();
}

bool Renderer::shouldClose() {
    return glfwWindowShouldClose(s_window);
}

int Renderer::init() {
    if (!glfwInit()) {
        return -1;
    }

    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    glfwWindowHint(GLFW_SAMPLES, 4);                                    // 4x MSAA
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 4);

    s_window = glfwCreateWindow(WINDOW_WIDTH, WINDOW_HEIGHT, "", NULL, NULL);

    if (!s_window) {
        glfwTerminate();
        return -1;
    }

    glfwSetWindowSizeCallback(s_window, windowSizeCallback);
    glfwSetKeyCallback(s_window, &Input::keyCallback);

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

    // Let driver decide what's better
    //glfwSwapInterval(0);

    if (init_gl() != 0 || loadData() != 0) {
        glfwTerminate();
        return -1;
    }

    uploadTextureHandles(s_textureHandleBufferID);
    glBindBufferBase(GL_UNIFORM_BUFFER, S_UBO_TEXTURES, s_textureHandleBufferID);

    Renderer::width = WINDOW_WIDTH;
    Renderer::height = WINDOW_HEIGHT;

    s_camera.uniformData.m_cameraPosition = glm::vec4(0.0f);
    s_camera.uniformData.m_cameraDestination = glm::vec4(1.0f, 0.0f, 0.0f, 0.0f);
    s_camera.uniformData.m_projectionMatrix = glm::perspective(45.0f, (float) WINDOW_WIDTH / (float) WINDOW_HEIGHT, 0.1f, 100.0f);
    s_camera.recalcMatrix();
    s_scene.init();

    return 0;
}

void Renderer::setCursorCallback(GLFWcursorposfun cbfun) {
    glfwSetCursorPosCallback(s_window, cbfun);
}

////

void Camera::translate(glm::vec3 delta) {
    uniformData.m_cameraPosition += glm::vec4(delta, 0);
    uniformData.m_cameraDestination += glm::vec4(delta, 0);
}

void Camera::recalcMatrix() {
    uniformData.m_cameraMatrix = glm::lookAt(
        glm::vec3(uniformData.m_cameraPosition),
        glm::vec3(uniformData.m_cameraDestination),
        { 0, 1, 0 }
    );
}
