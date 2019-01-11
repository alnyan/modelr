#pragma once
#include <GL/glew.h>
#include <glm/glm.hpp>

#define R_SHADOW_MAP_WIDTH      4096
#define R_SHADOW_MAP_HEIGHT     4096


#define S_ATTRIB_VERTEX         0
#define S_ATTRIB_TEXCOORD       1
#define S_ATTRIB_NORMAL         2
#define S_ATTRIB_TANGENT        3
#define S_ATTRIB_BITANGENT      4

#define S_UBO_SCENE             0
#define S_UBO_TEXTURES          1
#define S_UBO_LIGHT0            2

#define S_SSBO_MODEL            1
#define S_SSBO_MESH_ATTRIB      2

#define S_TEXTURE_COUNT         256
#define S_SHADOW_CASCADES       4
#define S_SHADOW_MAP_0          (S_TEXTURE_COUNT - S_SHADOW_CASCADES)

#define textureIndex(i)         ((i) * 2)

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
    glm::mat4 m_projectionMatrix[S_SHADOW_CASCADES];
    glm::mat4 m_lightMatrix;
    glm::vec4 m_lightPosition;
};

struct MeshAttrib {
    int material;
};
