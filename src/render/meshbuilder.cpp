#include "meshbuilder.h"
#include <iostream>

MeshBuilder::MeshBuilder(GLuint arrayID): m_arrayID{arrayID} {
    glGenBuffers(1, &m_geometryBufferID);
}

MeshBuilder::~MeshBuilder() {
    glDeleteBuffers(1, &m_geometryBufferID);
}

void MeshBuilder::begin() {
    std::cout << "Mesh building begun" << std::endl;
    m_vertexData.clear();
    m_vertexCount = 0;
}

void MeshBuilder::commit() {
    glBindVertexArray(m_arrayID);
    glBindBuffer(GL_ARRAY_BUFFER, m_geometryBufferID);
    glBufferData(GL_ARRAY_BUFFER, sizeof(MeshVertexAttrib) * m_vertexCount, &m_vertexData[0], GL_STATIC_DRAW);

    // Vertex data
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(MeshVertexAttrib), (GLvoid *) 0);
    // TexCoord data
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, sizeof(MeshVertexAttrib), (GLvoid *) sizeof(glm::vec3));
    // Normal data
    glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, sizeof(MeshVertexAttrib), (GLvoid *) (sizeof(glm::vec3) + sizeof(glm::vec2)));
    // Tangent data
    glVertexAttribPointer(3, 3, GL_FLOAT, GL_FALSE, sizeof(MeshVertexAttrib), (GLvoid *) (sizeof(glm::vec3) * 2 + sizeof(glm::vec2)));
    // Bitangent data
    glVertexAttribPointer(4, 3, GL_FLOAT, GL_FALSE, sizeof(MeshVertexAttrib), (GLvoid *) (sizeof(glm::vec3) * 3 + sizeof(glm::vec2)));

    glBindVertexArray(0);

    std::cout << "Mesh uploaded:" << std::endl;
    std::cout << (m_vertexCount / 3) << " triangles" << std::endl;
    // Free client memory
    m_vertexCount = 0;
    m_vertexData.clear();
}

GLuint MeshBuilder::beginShape() {
    m_marker = m_vertexCount;
    m_counter = 0;
    return m_vertexCount;
}

GLuint MeshBuilder::endShape() {
    return m_vertexCount - m_marker;
}

void MeshBuilder::normal(glm::vec3 n) {
    m_normal = n;
}

void MeshBuilder::texCoord(glm::vec2 t) {
    m_texCoord = t;
}

void MeshBuilder::vertex(glm::vec3 v) {
    MeshVertexAttrib m {
        v, m_texCoord, m_normal
        // TODO: generate tangent/bitangent
    };

    ++m_vertexCount;
    ++m_counter;
    m_vertexData.push_back(m);

    if (m_counter % 3 == 0) {
        glm::vec3 &v0 = m_vertexData[m_vertexCount - 3].vertex;
        glm::vec3 &v1 = m_vertexData[m_vertexCount - 2].vertex;
        glm::vec3 &v2 = m_vertexData[m_vertexCount - 1].vertex;

        glm::vec2 &t0 = m_vertexData[m_vertexCount - 3].texCoord;
        glm::vec2 &t1 = m_vertexData[m_vertexCount - 2].texCoord;
        glm::vec2 &t2 = m_vertexData[m_vertexCount - 1].texCoord;

        glm::vec3 dv0 = v1 - v0;
        glm::vec3 dv1 = v2 - v0;

        glm::vec2 dt0 = t1 - t0;
        glm::vec2 dt1 = t2 - t0;

        float r = 1.0f / (dt0.x * dt1.y - dt1.x * dt0.y);
        glm::vec3 t = -glm::normalize((dv0 * dt1.y - dv1 * dt0.y) * r);
        glm::vec3 b = -glm::normalize((dv1 * dt0.x - dv0 * dt1.x) * r);

#define round_r(v) \
        if (glm::abs(v) < 1e-7) v = 0

        round_r(t.x);
        round_r(t.y);
        round_r(t.z);
        round_r(b.x);
        round_r(b.y);
        round_r(b.z);

#undef round_r

        m_vertexData[m_vertexCount - 3].tangent = t;
        m_vertexData[m_vertexCount - 2].tangent = t;
        m_vertexData[m_vertexCount - 1].tangent = t;
        m_vertexData[m_vertexCount - 3].bitangent = b;
        m_vertexData[m_vertexCount - 2].bitangent = b;
        m_vertexData[m_vertexCount - 1].bitangent = b;

        auto n0 = m_vertexData[m_vertexCount - 3].normal;
        auto n1 = m_vertexData[m_vertexCount - 2].normal;
        auto n2 = m_vertexData[m_vertexCount - 1].normal;

        std::cout << "TBN:" << std::endl;
        std::cout << t.x << " " << t.y << " " << t.z << std::endl;
        std::cout << b.x << " " << b.y << " " << b.z << std::endl;
        std::cout << n0.x << " " << n0.y << " " << n0.z << std::endl;
    }
}
