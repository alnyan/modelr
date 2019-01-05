#pragma once
#include <GL/glew.h>
#include <glm/glm.hpp>
#include <list>
#include <string>
#include "material.h"
#include "shader.h"
#include <vector>

// Vertex attribute interleaved layout:
//
//      [N]     Attrib
//       0      x0
//       1      y0
//       2      z0
//       3      u0
//       4      v0
//       5      nx0
//       6      ny0
//       7      nz0
//

struct VertexBuffer {
    int flags = 0;
    GLuint vertexCount;
    GLuint vertexBufferID;
    GLuint tangentBufferID;
    GLuint arrayID;
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

    struct VertexFormat {
        glm::vec3 v;
        glm::vec2 n;
        glm::vec3 t;
    };

    struct TangentFormat {
        glm::vec3 tangent, bitangent;
    };
private:
    std::vector<VertexFormat> m_vertexData;
    std::vector<TangentFormat> m_tangentData;

    bool m_genTangents;
    GLfloat m_nx, m_ny, m_nz, m_u, m_v;
};

class Model {
public:
    Model(VertexBuffer b, Material *mt);
    ~Model();

    void render(Shader *shader, GLuint mode = GL_TRIANGLES);
    void bind(Shader *shader);
    void unbind(Shader *shader);
    static void unbindAll();

    static Model *loadObj(const std::string &path);

private:

    size_t m_vertexCount = 0;
    Material *m_material = nullptr;
    VertexBuffer m_buffer;
};
