#pragma once
#include <glm/glm.hpp>

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
     * @brief Shortcut for adding normals/texCoords/vertices for each of 3 vertices.
     */
    void triangle(glm::vec3 va, glm::vec2 ta, glm::vec3 na,
                  glm::vec3 vb, glm::vec2 tb, glm::vec3 nb,
                  glm::vec3 vc, glm::vec2 tc, glm::vec3 nc);

private:
    std::vector<MeshVertexAttrib> m_vertexData;

    GLint m_vertexCount;
    GLuint m_arrayID;
};
