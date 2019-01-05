#pragma once
#include <GL/glew.h>
#include <glm/glm.hpp>
#include <vector>

// Single VAO which contains all model meshes
//   Models are specified by their mesh offset

struct MeshVertexAttrib {
    glm::vec3 vertex;
    glm::vec2 texCoord;
    glm::vec3 normal;
    glm::vec3 tangent;
    glm::vec3 bitangent;
};

class MeshBuilder {
public:
    MeshBuilder(GLuint arrayID);
    ~MeshBuilder();

    /**
     * @brief Clear client state.
     */
    void begin();

    /**
     * @brief Configure array and upload vertexData to GPU, then clear client state.
     */
    void commit();

    void normal(glm::vec3 p);
    void vertex(glm::vec3 v);
    void texCoord(glm::vec2 t);

    /**
     * @brief Returns new shape offset
     */
    GLuint beginShape();

    /**
     * @brief Returns the size of the shape
     */
    GLuint endShape();

private:
    std::vector<MeshVertexAttrib> m_vertexData;

    GLuint m_marker;
    glm::vec3 m_normal;
    glm::vec2 m_texCoord;
    GLuint m_geometryBufferID;
    GLint m_vertexCount = 0;
    GLuint m_arrayID;
};
