#pragma once
#include <GL/glew.h>
#include <glm/glm.hpp>
#include <list>
#include <string>
#include "material.h"
#include "shader.h"
#include <vector>

struct VertexBuffer {
    int flags = 0;
    GLuint vertexCount;
    GLuint bufferIDs[5];
};

class MeshBuilder {
public:
    MeshBuilder(bool hasNormalMap = true);

    void genTangents();
    void uploadMesh();

    void texCoord(GLfloat u, GLfloat v);
    void normal(GLfloat nx, GLfloat ny, GLfloat nz);
    void vertex(GLfloat x, GLfloat y, GLfloat z);

    VertexBuffer buffer;
private:
    std::vector<GLfloat> m_vertexData,
                         m_texCoordData,
                         m_normalData,
                         m_tangentData,
                         m_bitangentData;

    bool m_genTangents;
    GLfloat m_nx, m_ny, m_nz, m_u, m_v;
};

class Model {
public:
    Model(VertexBuffer b, Material *mt);
    ~Model();

    void render(Shader *shader);

    static Model *loadObj(const std::string &path);

private:

    size_t m_vertexCount = 0;
    Material *m_material = nullptr;
    VertexBuffer m_buffer;
};
