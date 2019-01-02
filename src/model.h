#pragma once
#include <GL/glew.h>
#include <string>
#include "material.h"
#include "shader.h"

class Model {
public:
    Model(const float *vertices,
          const float *texCoords,
          const float *normals,
          Material *mat,
          size_t count);
    ~Model();

    void render(Shader *shader);

    static Model *loadObj(const std::string &path);

private:

    size_t m_vertexCount = 0;
    Material *m_material = nullptr;
    GLuint m_vertexBufferID = 0, m_normalBufferID = 0, m_texCoordBufferID = 0;
};
