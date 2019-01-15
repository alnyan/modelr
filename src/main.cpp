#include <iostream>
#include <math.h>
#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include "render/common.h"
#include "render/shader.h"
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include "gameobject.h"
#include "res/lodepng.h"
#include <algorithm>
#include <list>
#include <map>

#include "render/wavefront.h"
#include "gldynamicarray.h"

#include "config.h"

//

static GLFWwindow *s_window;

static constexpr float s_moveSpeed = 5;
static int m_width, m_height;

// Particles
static GLuint s_particleDataBufferID;
static Particle *s_particles = nullptr;
static GLuint s_particleGeometryArrayID;

// Dynamic objects
static GLuint s_sceneShaderID;
static GLuint s_allGeometryArrayID;
static GlDynamicArray<DrawArraysIndirectCommand, GL_DRAW_INDIRECT_BUFFER, 0xFF> s_indirectCommands;
static GlDynamicArray<MeshAttrib, GL_SHADER_STORAGE_BUFFER, S_SSBO_MESH_ATTRIB> s_meshAttribs;
static GlDynamicArray<glm::mat4, GL_SHADER_STORAGE_BUFFER, S_SSBO_MODEL> s_modelMatrices;
static GLuint s_sceneBillboardShaderID;

static std::vector<GameObject> s_objects;

static MeshBuilder *s_allGeometryBuilder;
static Model s_models[3];

// Resources
static GLuint s_textureHandleBufferID,
              s_materialBufferID;
static GLuint s_textureIDs[S_TEXTURE_COUNT];
static GLuint64 s_textureHandles[S_TEXTURE_COUNT * 2];
static MaterialUniformData s_materials[S_MATERIAL_COUNT];

// Scene-global data
static GLuint s_sceneUniformBufferID;
static GLuint s_sceneLight0UniformBufferID;
static SceneUniformData s_sceneUniformData;
static glm::vec3 s_cameraVelocity;
static float s_cameraPitch = 0, s_cameraYaw = 0;
static glm::vec2 s_cameraRotDelta;
static int s_walk = 0;
static int s_strafe = 0;
static int s_fly = 0;

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

//

static int s_lastTextureIndex = 0;
static int s_lastMaterialIndex = 0;
static std::map<std::string, int> s_materialBinding;
static std::map<std::string, int> s_textureBindings;

int loadTextureID(GLuint id, const std::string &path, GLuint minFilter = GL_LINEAR_MIPMAP_LINEAR, GLuint magFilter = GL_LINEAR) {
    unsigned int w, h;
    std::vector<unsigned char> data;

    if (lodepng::decode(data, w, h, path) != 0) {
        std::cerr << "Failed to load texture: " << path << std::endl;
        return -1;
    }

    glBindTexture(GL_TEXTURE_2D, id);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, w, h, 0, GL_RGBA, GL_UNSIGNED_BYTE, &data[0]);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, magFilter);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, minFilter);
    if (minFilter == GL_LINEAR_MIPMAP_LINEAR) {
        glGenerateMipmap(GL_TEXTURE_2D);
    }
    glBindTexture(GL_TEXTURE_2D, 0);

    return 0;
}

int loadTexture(int *i, const std::string &path) {
    std::cout << "Load " << path;
    auto it = s_textureBindings.find(path);
    if (it != s_textureBindings.end()) {
        if (i) {
            *i = it->second;
        }
        std::cout << ": cached" << std::endl;
        return 0;
    } else {
        int idx = s_lastTextureIndex++;
        s_textureBindings[path] = idx;
        if (idx >= S_TEXTURE_MAX) {
            std::cerr << "No free texture IDs left" << std::endl;
            return -1;
        }
        if (i) {
            *i = idx;
        }

        if (loadTextureID(s_textureIDs[idx], path) != 0) {
            std::cerr << ": failed" << std::endl;
            return -1;
        }

        std::cout << ": index " << idx << ", ID " << s_textureIDs[idx] << std::endl;

        return 0;
    }
}

int createMaterial(const std::string &name) {
    auto it = s_materialBinding.find(name);
    if (it != s_materialBinding.end()) {
        return it->second;
    } else {
        int idx = s_lastMaterialIndex++;
        if (idx >= S_MATERIAL_COUNT) {
            std::cerr << "Failed to allocate material" << std::endl;
            return -1;
        }

        MaterialUniformData *mat = &s_materials[idx];
        // Initial setup
        mat->m_maps[1] = S_TEXTURE_UNDEFINED;
        mat->m_maps[2] = S_TEXTURE_UNDEFINED;
        mat->m_maps[0] = S_TEXTURE_UNDEFINED;
        mat->m_Ks = glm::vec4(0.2, 0.2, 0.2, 100);

        return idx;
    }
}

int loadTextureMulti(int *indices, const std::list<std::string> &items) {
    int n = items.size();
    int i = 0;

    for (const auto &path: items) {
        if (loadTexture(&indices[i], path) != 0) {
            return -1;
        }
        ++i;
    }

    return 0;
}

int getTexture(const std::string &name) {
    auto it = s_textureBindings.find(name);
    if (it != s_textureBindings.end()) {
        return it->second;
    } else {
        return -1;
    }
}

int loadData(void) {
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

    glGenBuffers(1, &s_materialBufferID);
    // Create materials for the two models
    {
        MaterialUniformData *mat;
        int idx;
        if ((idx = createMaterial("Material0")) < 0) {
            std::cerr << "Failed to setup materials" << std::endl;
            return -1;
        }

        mat = &s_materials[idx];
        mat->m_maps[1] = getTexture("assets/normal.png");
        mat->m_maps[0] = getTexture("assets/texture.png");          // TODO: use actual linking
        mat->m_maps[2] = getTexture("assets/extension.png");
        mat->m_Ks.w = 10;

        if ((idx = createMaterial("Material1")) < 0) {
            std::cerr << "Failed to setup materials" << std::endl;
            return -1;
        }

        mat = &s_materials[idx];
        // LOL
        mat->m_maps[1] = getTexture("assets/terrain.png");
        mat->m_maps[0] = getTexture("assets/terrain.png");
        mat->m_Ks.w = 1e10;

        s_models[0].materialIndex = idx - 1;
        s_models[1].materialIndex = idx;             // No material, testing purposes
    }

    glBindBuffer(GL_UNIFORM_BUFFER, s_materialBufferID);
    glBufferData(GL_UNIFORM_BUFFER, sizeof(s_materials), &s_materials, GL_STATIC_DRAW);
    glBindBuffer(GL_UNIFORM_BUFFER, 0);
    glBindBufferBase(GL_UNIFORM_BUFFER, S_UBO_MATERIALS, s_materialBufferID);

    std::cout << "Loading complete" << std::endl;
    return 0;
}

int loadTextures(void) {
    int e;
    for (int i = 0; i < S_SHADOW_CASCADES; ++i) {
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

    //std::cout << "Velocity map " << 0 << ": " << s_sceneTextures[2] << std::endl;
    //s_textureHandles[textureIndex(S_VELOCITY_BUFFER)] = glGetTextureHandleARB(s_sceneTextures[2]);
    //if (glGetError()) {
        //std::cerr << "Failed to generate velocity map handle" << std::endl;
        //return -1;
    //}
    //glMakeTextureHandleResidentARB(s_textureHandles[textureIndex(S_VELOCITY_BUFFER)]);

    GLuint undefinedTextureID;
    glGenTextures(1, &undefinedTextureID);

    if (loadTextureID(undefinedTextureID, "assets/undefined.png", GL_NEAREST, GL_NEAREST) != 0) {
        std::cout << "Failed to load \"undefined\"" << std::endl;
        return -1;
    }

    GLuint undefinedTextureHandle = glGetTextureHandleARB(undefinedTextureID);
    if (glGetError()) {
        std::cerr << "Failed to get undefined texture handle" << std::endl;
        return -1;
    }
    glMakeTextureHandleResidentARB(undefinedTextureHandle);
    s_textureHandles[textureIndex(S_TEXTURE_UNDEFINED)] = undefinedTextureHandle;

    glGenTextures(5, s_textureIDs);
    std::list<std::string> textures {
        "assets/texture.png",
        "assets/normal.png",
        "assets/terrain.png",
        "assets/smoke.png",
        "assets/extension.png"
    };
    int *indices = new int[5];

    if (loadTextureMulti(indices, textures) != 0) {
        delete indices;
        return -1;
    }

    for (int j = 0; j < 5; ++j) {
        int i = indices[j];
        GLuint texID = s_textureIDs[i];
        GLuint texHandle = glGetTextureHandleARB(texID);
        if ((e = glGetError())) {
            std::cerr << "Failed to get texture handle for " << i << std::endl;
            return -1;
        }

        glMakeTextureHandleResidentARB(texHandle);

        s_textureHandles[textureIndex(i)] = texHandle;
    }

    delete indices;

    glGenBuffers(1, &s_textureHandleBufferID);

    glBindBuffer(GL_UNIFORM_BUFFER, s_textureHandleBufferID);
    glBufferData(GL_UNIFORM_BUFFER, sizeof(s_textureHandles), s_textureHandles, GL_STATIC_DRAW);
    glBindBuffer(GL_UNIFORM_BUFFER, 0);
    glBindBufferBase(GL_UNIFORM_BUFFER, S_UBO_TEXTURES, s_textureHandleBufferID);

    return 0;
}



void rmObject(int index) {
    const auto &obj = s_objects[index];

    s_indirectCommands.remove(obj.dataIndex);
    s_meshAttribs.remove(obj.dataIndex);
    s_modelMatrices.remove(obj.dataIndex);

    s_objects.erase(s_objects.begin() + index);
    for (int i = index; i < s_objects.size(); ++i) {
        --s_objects[i].dataIndex;
    }
}

void addObject(int modelID, glm::vec3 pos, glm::vec3 vel, glm::vec3 rot) {
    int index = s_indirectCommands.size;

    s_indirectCommands.append({ s_models[modelID].size, 1, s_models[modelID].begin, 0 });
    s_meshAttribs.append({ s_models[modelID].materialIndex, {}, glm::vec4(vel, 0) });
    s_modelMatrices.append(glm::rotate(glm::translate(glm::mat4(1), pos), rot.y, { 0, 1, 0 }));

    s_objects.push_back({
        pos,
        vel,
        rot,
        index
    });
}

int init(void) {
    s_sceneUniformData.m_cameraPosition = glm::vec4(2, 2, 2, 1);
    s_sceneUniformData.m_cameraDestination = glm::vec4(2, 2, 3, 1);

    s_indirectCommands.generate();
    s_meshAttribs.generate();
    s_modelMatrices.generate();

    // Setup drawcalls
    addObject(1, { 0, 0, 0 }, { 0, 0, 0 }, { 0, 0, 0 });
    addObject(0, { 20, 20, 20 }, { 0, 0, 0 }, { 0, 0, 0 });

    for (int i = 0; i < 100; ++i) {
        addObject(0, { rand() / (float) RAND_MAX * 10, rand() / (float) RAND_MAX * 10, rand() / (float) RAND_MAX * 10 }, { 0, 0, 0 }, { 0, rand() / (float) RAND_MAX, 0 });
    }

    return 0;
}


int setup_gl(void) {
    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LESS);

    // Post-processing and render to texture setup
    glGenFramebuffers(1, &s_sceneBuffer);
    glGenTextures(4, s_sceneTextures);
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
    glBindTexture(GL_TEXTURE_2D, s_sceneTextures[2]);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB16F, 800, 600, 0, GL_RGB, GL_FLOAT, 0);
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
        s_cameraVelocity = 0.01f * glm::vec3(walkDelta) + glm::vec3(s_cameraRotDelta, 0);

        s_sceneUniformData.m_cameraPosition += walkDelta;
        s_sceneUniformData.m_cameraDestination += walkDelta;
    } else {
        s_cameraVelocity = glm::vec3(s_cameraRotDelta, 0);
    }
    s_cameraRotDelta = glm::mix(glm::vec2(0), s_cameraRotDelta, 0.3);

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

void addParticle(glm::vec3 pos, glm::vec3 vel, float t0, float t) {
    int i = s_lastParticle++;
    s_lastParticle %= R_PARTICLE_MAX;

    s_particles[i] = {
        glm::vec4(pos, 0), glm::vec4(vel, 0), glm::ivec4(0), t0, t
    };
}

void update(double t, double dt) {
    updatePlayer(t, dt);
    updateLight0(t);

    if (s_phys) {
    //if (t - s_lastObjectTime > 3) {
        //s_lastObjectTime = t;
        //for (int i = 0; i < 5; ++i)
        //if (s_indirectCommands.size > 1)
            //rmObject(1);
        //std::cout << s_indirectCommands.size << " objects" << std::endl;
    //}

    if (t - s_lastParticleTime > 0.05) {
        s_lastParticleTime = t;
        auto r0 = rand() / (float) RAND_MAX;
        auto r1 = rand() / (float) RAND_MAX;
        auto r2 = rand() / (float) RAND_MAX;
        auto r3 = rand() / (float) RAND_MAX;

        float y = 0.06;
        float sx = 0.005;
        float sy = 0.1;

        addParticle({ 6 + r2 * sy * 2 - sy, 1, 6 + r3 * sy * 2 - sy }, { r0 * sx * 2 - sx, y, r1 * sx * 2 - sx }, t, 6);
    }

    for (int i = 0; i < R_PARTICLE_MAX; ++i) {
        Particle &p = s_particles[i];

        if ((t - p.t0) - p.t > 0 || p.t < 1 || p.t0 == 0) {
        } else {
            p.pos += p.vel;
            p.vel *= 0.9998;
        }
    }

    for (auto &obj: s_objects) {
        if (obj.dataIndex < 2) {
            s_meshAttribs[obj.dataIndex].vel = glm::vec4(0);
            continue;
        }
        obj.pos += obj.vel * (float) dt;
        obj.rot = glm::vec3(0, glm::cos(t), 0);
        glm::vec3 f = glm::normalize(obj.pos) / (float) glm::pow(glm::length(obj.pos), 2);
        obj.vel += f * (float) dt;
        s_modelMatrices[obj.dataIndex] = glm::rotate(glm::translate(glm::mat4(1), obj.pos), obj.rot.y, { 0, 1, 0 });
        s_meshAttribs[obj.dataIndex].vel = glm::vec4(obj.vel, 1);
    }

    }
}

////

void renderScene(void) {
    auto t0 = glfwGetTime();
    glUseProgram(s_sceneShaderID);
    auto l = glGetUniformLocation(s_sceneShaderID, "mCameraVelocity");
    glUniform3f(l, s_cameraVelocity.x, s_cameraVelocity.y, s_cameraVelocity.z);
    auto t1 = glfwGetTime();
    glMultiDrawArraysIndirect(s_renderType ? GL_LINES : GL_TRIANGLES, 0, s_indirectCommands.size, 0);
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
    glMultiDrawArraysIndirect(s_renderType ? GL_LINES : GL_TRIANGLES, 0, s_indirectCommands.size, 0);
    s_drawCallTime += glfwGetTime() - t1;
    s_transferTime += t1 - t0;
}

void renderScreen(void) {
    auto t0 = glfwGetTime();
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glViewport(0, 0, m_width, m_height);
    glUseProgram(s_screenShaderID);
    auto l = glGetUniformLocation(s_screenShaderID, "mDepthTexture");
    glUniform1i(l, 1);
    l = glGetUniformLocation(s_screenShaderID, "mVelocityTexture");
    glUniform1i(l, 2);
    l = glGetUniformLocation(s_screenShaderID, "mScreenDimensions");
    glm::vec4 screenDimensions(m_width, m_height, 0.1f, 100.0f);
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

void render(void) {
    auto t = glfwGetTime();
    auto dt = t - s_lastTime;
    s_lastTime = t;

    update(t, dt);

    auto t0 = glfwGetTime();
    glNamedBufferData(s_sceneUniformBufferID, sizeof(SceneUniformData), &s_sceneUniformData, GL_DYNAMIC_DRAW);
    glNamedBufferData(s_sceneLight0UniformBufferID, sizeof(SceneLight0UniformData), &s_light0UniformData, GL_DYNAMIC_DRAW);

    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glBindTexture(GL_TEXTURE_2D, 0);

    // BEGIN RENDER
    // BLAST 1: LIGHT'S ORTHO
    glBindVertexArray(s_allGeometryArrayID);
    glBindBuffer(GL_DRAW_INDIRECT_BUFFER, s_indirectCommands.bufferID);
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
    glViewport(0, 0, m_width, m_height);
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

    glBindVertexArray(s_particleGeometryArrayID);
    glEnableVertexAttribArray(S_PARTICLE_ATTRIB_VERTEX);
    glEnableVertexAttribArray(S_PARTICLE_ATTRIB_TEXCOORD);
    glUseProgram(s_sceneBillboardShaderID);
    glEnable(GL_BLEND);
    //glBlendFunc(GL_SRC_ALPHA, GL_ONE);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glDepthMask(GL_FALSE);
    auto l = glGetUniformLocation(s_sceneBillboardShaderID, "mTime");
    glUniform1f(l, t);
    auto t5 = glfwGetTime();
    glDrawArraysInstancedBaseInstance(GL_TRIANGLES, 0, 6, R_PARTICLE_MAX, 0);
    auto t6 = glfwGetTime();
    glDepthMask(GL_TRUE);
    glDisable(GL_BLEND);

    glBindVertexArray(0);
    glDisableVertexAttribArray(S_PARTICLE_ATTRIB_VERTEX);
    glDisableVertexAttribArray(S_PARTICLE_ATTRIB_TEXCOORD);

    // END RENDER
    glUseProgram(0);

    auto t7 = glfwGetTime();
    renderScreen();

    auto r = glfwGetTime();

    s_frameTimeSum += r - t;
    s_transferTime += (t7 - t6) + (t5 - t4) + (t3 - t2) + (t1 - t0);
}

//

void windowSizeCallback(GLFWwindow *win, int width, int height) {
    m_width = width;
    m_height = height;

    glBindTexture(GL_TEXTURE_2D, s_sceneTextures[0]);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB16F, m_width, m_height, 0, GL_RGB, GL_FLOAT, 0);
    glBindTexture(GL_TEXTURE_2D, s_sceneTextures[1]);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT24, m_width, m_height, 0, GL_DEPTH_COMPONENT, GL_FLOAT, 0);
    glBindTexture(GL_TEXTURE_2D, s_sceneTextures[2]);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB16F, m_width, m_height, 0, GL_RGB, GL_FLOAT, 0);
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
    s_cameraRotDelta = glm::mix(s_cameraRotDelta, glm::vec2(s_cameraYaw, 0) + s_cameraRotDelta, 0.2);
}

void keyCallback(GLFWwindow *win, int key, int scan, int action, int mods) {
    if (key == GLFW_KEY_T && action == 1) {
        ++s_viewType;
        s_viewType %= 6;
    }
    if (key == GLFW_KEY_EQUAL && action == 1) {
        ++s_shadowControl;
    }
    if (key == GLFW_KEY_MINUS && action == 1) {
        --s_shadowControl;
    }
    if (key == GLFW_KEY_R && action == 1) {
        ++s_renderType;
        s_renderType %= 2;
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

    if (key == GLFW_KEY_P && action == 1) {
        s_phys = !s_phys;
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
    double swapTime = 0;
    int frames = 0;

    while (!glfwWindowShouldClose(s_window)) {
        auto t1 = glfwGetTime();
        if (t1 - t0 > 1) {
            std::cout << frames << " frames" << std::endl;
            std::cout << s_frameTimeSum * 1000 << " ms" << std::endl;
            std::cout << (s_frameTimeSum / frames) * 1000 << " ms/frame" << std::endl;
            std::cout << (s_drawCallTime / frames) * 1000 << " ms/draw" << std::endl;
            std::cout << (s_transferTime / frames) * 1000 << " ms/xfer" << std::endl;
            std::cout << (swapTime / frames) * 1000 << " ms/swap" << std::endl;
            t0 = t1;
            frames = 0;
            s_frameTimeSum = 0;
            s_drawCallTime = 0;
            s_transferTime = 0;
            swapTime = 0;
        }

        render();
        ++frames;

        t1 = glfwGetTime();
        glfwSwapBuffers(s_window);
        swapTime += glfwGetTime() - t1;
        glfwPollEvents();
    }

    glfwTerminate();

    return 0;
}
