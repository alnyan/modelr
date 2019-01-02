#pragma once
#include <GL/glew.h>
#include <glm/glm.hpp>
#include <list>
#include <string>
#include "material.h"
#include "shader.h"
#include <vector>

class MeshBuilder {
public:
    MeshBuilder();

    void uploadMesh();

    void texCoord(GLfloat u, GLfloat v);
    void normal(GLfloat nx, GLfloat ny, GLfloat nz);
    void vertex(GLfloat x, GLfloat y, GLfloat z);

    GLuint vertexBufferID, texCoordBufferID, normalBufferID;
    size_t vertexCount = 0;
private:
    std::vector<GLfloat> m_vertexData, m_texCoordData, m_normalData;
    GLfloat m_nx, m_ny, m_nz, m_u, m_v;
};

class Model {
public:
    Model(size_t sz, GLuint vid, GLuint tid, GLuint nid, Material *mt);
    ~Model();

    void render(Shader *shader);

    static Model *loadObj(const std::string &path);

private:

    size_t m_vertexCount = 0;
    Material *m_material = nullptr;
    GLuint m_vertexBufferID = 0, m_normalBufferID = 0, m_texCoordBufferID = 0;
};
