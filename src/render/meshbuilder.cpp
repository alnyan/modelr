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

    m_vertexData.push_back(m);
    ++m_vertexCount;
}
